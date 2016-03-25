#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>

#include "./config.h"

#define RED_ZONE_BYTES 128

static_assert(RED_ZONE_BYTES >= sizeof(struct rlimit),
              "rlimit too big to use red zone");

DEFINE_int32(resource, RLIMIT_CORE,
             "the resource to raise -- by default, RLIMIT_CORE");

class PtraceScope {
 public:
  PtraceScope() = delete;
  explicit PtraceScope(pid_t pid) : pid_(pid), attached_(false) {}

  ~PtraceScope() {
    if (attached_) {
      SetRegs(orig_regs_);
      Poke(orig_regs_.rip, orig_word_);
      Detach();
    }
  }

  void Attach() {
    CHECK(!attached_);
    PLOG_IF(FATAL, ptrace(PTRACE_ATTACH, pid_, 0, 0))
        << "failed to PTRACE_ATTACH";

    int status;
    pid_t actual = wait(&status);
    CHECK(actual == pid_);
    attached_ = true;
  }

  void Detach() {
    PLOG_IF(FATAL, ptrace(PTRACE_DETACH, pid_, 0, 0))
        << "failed to PTRACE_DETACH";
  }

  void Start() {
    CHECK(attached_);

    LOG(INFO) << "getting original registers";
    GetRegs(&orig_regs_);
    LOG(INFO) << "getting original word at " << (void *)orig_regs_.rip;
    orig_word_ = Peek(orig_regs_.rip);

    LOG(INFO) << "preparing for syscalls" << sizeof(struct rlimit);
    // Poke(orig_regs_.rip, 0xf050);
    Poke(orig_regs_.rip, 0x050f);
    LOG_IF(FATAL, GetRlimit(FLAGS_resource)) << "failed to getrlimit()";

    struct rlimit rlimit;
    for (size_t i = 0; i < sizeof(struct rlimit); i += sizeof(long)) {
      long rslt = Peek(orig_regs_.rsp - sizeof(struct rlimit) + i);
      memcpy(&rlimit + i, &rslt, sizeof(rslt));
    }
    LOG(INFO) << "rlim_cur = " << rlimit.rlim_cur
              << ", rlim_max = " << rlimit.rlim_max;

    SetRegs(orig_regs_);
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
    regs.rax = 97;  // sys_getrlimit
    regs.rdi = (unsigned long)resource;
    regs.rsi = orig_regs_.rsp - sizeof(struct rlimit);
    SetRegs(regs);

    user_regs_struct after;
    SingleStep(&after);
    CHECK(after.rip == orig_regs_.rip + 2);
    return after.rax;
  }

  void GetRegs(user_regs_struct *s) {
    CHECK(attached_);
    PLOG_IF(FATAL, ptrace(PTRACE_GETREGS, pid_, 0, s))
        << "failed to PTRACE_GETREGS";
  }

  void SetRegs(const user_regs_struct &s) {
    CHECK(attached_);
    PLOG_IF(FATAL, ptrace(PTRACE_SETREGS, pid_, 0, &s))
        << "failed to PTRACE_SETREGS";
  }

  long Peek(unsigned long where) {
    DVLOG(1) << "peeking at " << (void *)where;
    errno = 0;
    long ret = ptrace(PTRACE_PEEKTEXT, pid_, where, 0);
    if (ret == -1 && errno != 0) {
      PLOG(FATAL) << "ptrace(PTRACE_PEEKTEXT, ..., " << (void *)where << ")";
    }
    return ret;
  }

  void Poke(unsigned long where, long value) {
    PLOG_IF(FATAL, ptrace(PTRACE_POKETEXT, pid_, where, value))
        << "failed to PTRACE_POKETEXT";
  }

 private:
  bool attached_;
  const pid_t pid_;

  user_regs_struct orig_regs_;
  long orig_word_;
};

int main(int argc, char **argv) {
  gflags::SetUsageMessage("[OPTIONS] PID");
  gflags::SetVersionString(VERSION);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

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
  scope.GetRlimit(FLAGS_resource);

  return 0;
}
