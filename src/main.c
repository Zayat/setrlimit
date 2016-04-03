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

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/user.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#ifdef HAVE_CONFIG_H
#include "./config.h"
#endif

#include "./ulog.h"
#include "./rlim.h"

static void do_wait(pid_t pid) {
  int status;
  pid_t waited = waitpid(pid, &status, 0);
  if (waited == (pid_t)-1) {
    perror("waitpid()");
    exit(1);
  }
  assert(waited != (pid_t)-1);
  assert(waited == pid);

  if (WIFEXITED(status)) {
    ulog_err("unfortunately pid %d decided to exit", pid);
  }
  if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    ulog_err("process was signalled via signal %d: %s", sig, strsignal(sig));
  }
  if (WIFSTOPPED(status)) {
    assert(WSTOPSIG(status) == SIGTRAP);
    ulog_info("status & 0x80 = %x", status & 0x80);
    ulog_info("status & (SIGTRAP | 0x80) = %x", status & (SIGTRAP | 0x80));
  } else {
    ulog_err("pid %d did not stop", pid);
  }
}

int main(int argc, char **argv) {
  int c;
  int resource = -1;
  bool verbose = false;
  bool do_list = false;
  opterr = 0;
  char *resource_str = NULL;

  while ((c = getopt(argc, argv, "hlr:vV")) != -1) {
    switch (c) {
      case 'h':
        goto usage;
        break;
      case 'l':
        do_list = true;
        break;
      case 'r':
        resource_str = optarg;  // not a copy!
        break;
      case 'v':
        verbose = true;
        break;
      case 'V':
        puts(PACKAGE_STRING);
        return 0;
        break;

      case '?':
        if (isprint(optopt)) {
          ulog_err("unexpectedly got option %c", optopt);
        } else {
          ulog_err("unknown option character `\\x%x'", optopt);
        }
        break;
    }
  }

  ulog_init(verbose);

  if (do_list) {
    print_rlimits();
    return 0;
  }

  if (argc - optind != 1) {
    goto usage;
  }

  if (resource_str != NULL) {
    resource = rlimit_by_name(resource_str);
    if (resource == -1) {
      errno = 0;
      resource = strtol(argv[optind], NULL, 10);
    }
  }

  if (resource < 0) {
    ulog_info("reluctantly setting resource to RLIMIT_CORE");
    resource = RLIMIT_CORE;
  }

  ulog_info("final value for resource is %d", resource);

  errno = 0;
  long maybe_pid = strtol(argv[optind], NULL, 10);
  if (errno) {
    perror("failed to call strtol()");
  }
  if (maybe_pid < 1) {
    ulog_err("cowardly refusing to trace pid %ld", maybe_pid);
  }

  pid_t pid = (pid_t)maybe_pid;
  ulog_info("pid is %d", pid);
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
  ulog_info("orig.rip = %p", orig.rip);

  errno = 0;
  const long orig_word = ptrace(PTRACE_PEEKTEXT, pid, orig.rip, 0);
  if (orig_word == -1 && errno) {
    perror("ptrace(PTRACE_PEEKTEXT, ...)");
    return 1;
  }

  ulog_info("orig word is %ld", orig_word);

  if (ptrace(PTRACE_POKETEXT, pid, orig.rip, 0x050f)) {
    perror("ptrace(PTRACE_POKETEXT, ...)");
    return 1;
  }

  ulog_info("poked text to prepare for syscall");

  struct user_regs_struct new_regs;
  memcpy(&new_regs, &orig, sizeof(new_regs));

  new_regs.rax = SYS_getrlimit;                         // sys_getrlimit
  new_regs.rdi = resource;                              // resource
  new_regs.rsi = new_regs.rsp - sizeof(struct rlimit);  // rlim

  ulog_info("setting regs for syscall");
  if (ptrace(PTRACE_SETREGS, pid, 0, &new_regs)) {
    perror("ptrace(PTRACE_SETREGS, ...)");
    return 1;
  }

  ulog_info("single setting through sys_getrlimit");
  if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0)) {
    perror("ptrace(PTRACE_SINGLESTEP, ...)");
    return 1;
  }

  do_wait(pid);

  ulog_info("single step succeeded, trying to get regs");
  if (ptrace(PTRACE_GETREGS, pid, 0, &new_regs)) {
    perror("ptrace(PTRACE_GETREGS, ...)");
    return 1;
  }

  if (new_regs.rip - 2 != orig.rip) {
    ulog_err("expected rip to be increased by 2, instead the delta is %d",
             new_regs.rip - orig.rip);
  }
  if (new_regs.rax != 0) {
    ulog_err("after kernel call rax != 0, rax = %d", (long)new_regs.rax);
  }

  struct rlimit rlim;
  read_rlimit(pid, new_regs.rsp - sizeof(struct rlimit), &rlim);
  ulog_info("rlim.rlim_cur = %ld, rlim.rlim_max = %ld", rlim.rlim_cur,
            rlim.rlim_max);

  if (rlim.rlim_cur == rlim.rlim_max) {
    ulog_info("both rlim_cur and rlim_max are %ld, nothing more to do",
              rlim.rlim_cur);
  } else {
    rlim.rlim_cur = rlim.rlim_max;
    poke_rlimit(pid, orig.rsp - sizeof(struct rlimit), &rlim);

    memcpy(&new_regs, &orig, sizeof(new_regs));
    new_regs.rax = SYS_setrlimit;                         // sys_setrlimit
    new_regs.rdi = resource;                              // resource
    new_regs.rsi = new_regs.rsp - sizeof(struct rlimit);  // rlim

    ulog_info("setting regs for syscall");
    if (ptrace(PTRACE_SETREGS, pid, 0, &new_regs)) {
      perror("ptrace(PTRACE_SETREGS, ...)");
      return 1;
    }

    ulog_info("single setting through sys_getrlimit");
    if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0)) {
      perror("ptrace(PTRACE_SINGLESTEP, ...)");
      return 1;
    }

    do_wait(pid);

    ulog_info("single step succeeded, trying to get regs");
    if (ptrace(PTRACE_GETREGS, pid, 0, &new_regs)) {
      perror("ptrace(PTRACE_GETREGS, ...)");
      return 1;
    }

    if (new_regs.rip - 2 != orig.rip) {
      ulog_err("expected rip to be increased by 2, instead the delta is %d",
               new_regs.rip - orig.rip);
    }
    if (new_regs.rax != 0) {
      ulog_err("after kernel call rax != 0, rax = %d", (long)new_regs.rax);
    }
  }

  ulog_info("restoring process to original state");

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

usage:
  fprintf(stderr, "usage: %s: PID\n", argv[0]);
  return 1;
}
