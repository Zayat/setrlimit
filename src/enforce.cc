// Copyright Evan Klitzke <evan@eklitzke.org>, 2016
//
// This file is part of setrlimit.
//
// Setrlimit is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Setrlimit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Setrlimit.  If not, see <http://www.gnu.org/licenses/>.

#include "./enforce.h"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/user.h>

#include <glog/logging.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#include "./rlim.h"

static void do_wait(pid_t pid) {
  int status;
  LOG(INFO) << "calling waitpid on pid " << pid;
  pid_t waited = waitpid(pid, &status, 0);
  if (waited == (pid_t)-1) {
    LOG(WARNING) << "failed to waitpid on pid " << pid;
    return;
  }
  assert(waited != (pid_t)-1);
  assert(waited == pid);

  if (WIFEXITED(status)) {
    LOG(FATAL) << "pid " << pid << " decided to exit";
  }
  if (WIFSIGNALED(status)) {
    LOG(WARNING) << "pid " << pid << " received signal " << WTERMSIG(status);
  }
  if (WIFSTOPPED(status)) {
    assert(WSTOPSIG(status) == SIGTRAP);
    LOG(INFO) << "pid " << pid << " stopped due to SIGTRAP";
    LOG(INFO) << "status & 0x80 = " << std::hex << (status & 0x80);
    LOG(INFO) << "status & (SIGTRAP | 0x80) = " << std::hex
              << (status & (SIGTRAP | 0x80));
  } else {
    LOG(FATAL) << "pid " << pid << " did not stop";
  }
}

int enforce(pid_t pid, int resource) {
  LOG(INFO) << "pid is " << pid;
  if (ptrace(PTRACE_SEIZE, pid, 0, PTRACE_O_TRACESYSGOOD)) {
    perror("ptrace(PRACE_SEIZE, ...)");
    return 1;
  }

  if (ptrace(PTRACE_INTERRUPT, pid, 0, 0)) {
    perror("ptrace(PRACE_INTERRUPT, ...)");
    return 1;
  }

  do_wait(pid);

  struct user_regs_struct orig;
  if (ptrace(PTRACE_GETREGS, pid, 0, &orig)) {
    perror("ptrace(PTRACE_GETREGS, ...)");
    return 1;
  }
  LOG(INFO) << "orig.rip = " << (void*)orig.rip;

  errno = 0;
  const long orig_word = ptrace(PTRACE_PEEKTEXT, pid, orig.rip, 0);
  if (orig_word == -1 && errno) {
    perror("ptrace(PTRACE_PEEKTEXT, ...)");
    return 1;
  }

  LOG(INFO) << "orig_word is " << orig_word;

  if (ptrace(PTRACE_POKETEXT, pid, orig.rip, 0x050f)) {
    perror("ptrace(PTRACE_POKETEXT, ...)");
    return 1;
  }

  LOG(INFO) << "poked text to prepare for syscall";

  struct user_regs_struct new_regs;
  memcpy(&new_regs, &orig, sizeof(new_regs));

  new_regs.rax = SYS_getrlimit;                         // sys_getrlimit
  new_regs.rdi = resource;                              // resource
  new_regs.rsi = new_regs.rsp - sizeof(struct rlimit);  // rlim

  LOG(INFO) << "setting regs for syscall";
  if (ptrace(PTRACE_SETREGS, pid, 0, &new_regs)) {
    perror("ptrace(PTRACE_SETREGS, ...)");
    return 1;
  }

  LOG(INFO) << "SINGLESTEPing through syscall";
  if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0)) {
    perror("ptrace(PTRACE_SINGLESTEP, ...)");
    return 1;
  }

  do_wait(pid);

  LOG(INFO) << "SINGLESTEP succeeded, trying to get regs";
  if (ptrace(PTRACE_GETREGS, pid, 0, &new_regs)) {
    perror("ptrace(PTRACE_GETREGS, ...)");
    return 1;
  }

  if (new_regs.rip - 2 != orig.rip) {
    LOG(FATAL) << "expected rip to be increased by 2, instead the delta is %d"
               << new_regs.rip - orig.rip;
  }
  if (new_regs.rax != 0) {
    LOG(FATAL) << "after kernel call rax != 0, rax = %d" << (long)new_regs.rax;
  }

  struct rlimit rlim;
  read_rlimit(pid, new_regs.rsp - sizeof(struct rlimit), &rlim);
  LOG(INFO) << "rlim.rlim_cur = " << rlim.rlim_cur
            << ", rlim.rlim_max = " << rlim.rlim_max;

  if (rlim.rlim_cur == rlim.rlim_max) {
    LOG(INFO) << "both rlim_cur and rlim_max are " << rlim.rlim_cur
              << ", mothing more to do";
  } else {
    rlim.rlim_cur = rlim.rlim_max;
    poke_rlimit(pid, orig.rsp - sizeof(struct rlimit), &rlim);

    memcpy(&new_regs, &orig, sizeof(new_regs));
    new_regs.rax = SYS_setrlimit;                         // sys_setrlimit
    new_regs.rdi = resource;                              // resource
    new_regs.rsi = new_regs.rsp - sizeof(struct rlimit);  // rlim

    LOG(INFO) << "setting regs for syscall";
    if (ptrace(PTRACE_SETREGS, pid, 0, &new_regs)) {
      perror("ptrace(PTRACE_SETREGS, ...)");
      return 1;
    }

    LOG(INFO) << "SINGLESTEPPing through sys_getrlimit";
    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0)) {
      perror("ptrace(PTRACE_SINGLESTEP, ...)");
      return 1;
    }

    do_wait(pid);

    LOG(INFO) << "SINGLESTEPP succeeded, trying to get regs";
    if (ptrace(PTRACE_GETREGS, pid, 0, &new_regs)) {
      perror("ptrace(PTRACE_GETREGS, ...)");
      return 1;
    }

    if (new_regs.rip - 2 != orig.rip) {
      LOG(FATAL)
          << "expected %rip to be increased by 2, instead the delta is %d"
          << new_regs.rip - orig.rip;
    }
    if (new_regs.rax != 0) {
      LOG(FATAL) << "after kernel call rax != 0, rax = %d"
                 << (long)new_regs.rax;
    }
  }

  LOG(INFO) << "restoring process to original state";

  if (ptrace(PTRACE_POKETEXT, pid, orig.rip, orig_word)) {
    perror("ptrace(PTRACE_POKETEXT...");
    return 1;
  }
  if (ptrace(PTRACE_SETREGS, pid, 0, &orig)) {
    perror("ptrace(PTRACE_SETREGS...");
    return 1;
  }
  if (ptrace(PTRACE_DETACH, pid, 0, 0)) {
    perror("ptrace(PTRACE_DETACH...");
    return 1;
  }
  return 0;
}
