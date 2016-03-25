#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <glog/logging.h>

#include <iostream>

class PtraceScope {
 public:
  PtraceScope() = delete;
  explicit PtraceScope(pid_t pid) : pid_(pid), attached_(false) {}

  void Attach() {
    CHECK(!attached_);
    PLOG_IF(FATAL, ptrace(PTRACE_ATTACH, pid_, 0, 0))
        << "failed to PTRACE_ATTACH";

    int status;
    pid_t actual = wait(&status);
    CHECK(actual == pid_);
    attached_ = true;
  }

  void Start() {
    CHECK(attached_);
    GetRegs(&orig_regs_);
    orig_word_ = Peek(orig_regs_.rip);
    Poke(orig_regs_.rip, 0xf050);
    unsigned long addr = Malloc();
  }

  unsigned long Malloc() {
    user_regs_struct regs;
    memcpy(&regs, &orig_regs_, sizeof(regs));
    regs.rax = 9;                            // sys_rlimit
    regs.rdi = 0;                            // addr
    regs.rsi = 4096;                         // length
    regs.rdx = PROT_READ | PROT_WRITE;       // prot
    regs.r10 = MAP_PRIVATE | MAP_ANONYMOUS;  // flags
    regs.r8 = -1;                            // fd
    regs.r9 = 0;                             // offset

    user_regs_struct after;
    SingleStep(&after);
    CHECK(after.rip == orig_regs_.rip + 2);
    LOG(INFO) << "malloced at " << (void *)after.rax;
    return after.rax;
  }

  int SingleStep(user_regs_struct *regs) {
    PLOG_IF(FATAL, ptrace(PTRACE_SINGLESTEP, pid_, 0, 0))
        << "failed to PTRACE_GETREGS";

    int status;
    pid_t actual = wait(&status);
    CHECK(actual == pid_);
    if (regs != nullptr) {
      GetRegs(regs);
    }
    return status;
  }

  int GetRlimit(int resource) {
    user_regs_struct regs;
    memcpy(&regs, &orig_regs_, sizeof(regs));
    regs.rax = 97;  // sys_rlimit
  }

  void GetRegs(user_regs_struct *s) {
    CHECK(attached_);
    PLOG_IF(FATAL, ptrace(PTRACE_GETREGS, pid_, 0, s))
        << "failed to PTRACE_GETREGS";
  }

  long Peek(unsigned long where) {
    errno = 0;
    long ret = ptrace(PTRACE_PEEKTEXT, pid_, 0, where);
    if (ret == -1 && errno) {
      PLOG(FATAL) << "ptrace(PTRACE_PEEKTEXT, ..., " << (void *)where << ")";
    }
    return ret;
  }

  void Poke(unsigned long where, long value) {
    PLOG_IF(FATAL, ptrace(PTRACE_POKETEXT, pid_, value, where))
        << "failed to PTRACE_POKETEXT";
  }

 private:
  bool attached_;
  const pid_t pid_;

  user_regs_struct orig_regs_;
  long orig_word_;
};

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);

  struct rlimit rlim;
  if (argc != 2) {
    LOG(ERROR) << "usage: " << argv[0] << " PID\n";
    return 1;
  }

  long val = strtol(argv[1], nullptr, 10);
  if (val <= 0) {
    std::cerr << "cowardly refusing to trace pid " << val << "\n";
  }

  PtraceScope scope(val);
  scope.Attach();
  scope.Start();
  scope.GetRlimit(RLIMIT_CORE);

  return 0;
}
