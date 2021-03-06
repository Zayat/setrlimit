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

#include <ctype.h>
#include <errno.h>
#include <glog/logging.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>

#include <memory>

static const char *tbl[] = {"CPU",   "FSIZE",   "DATA",  "STACK",
                            "CORE",  "RSS",     "NPROC", "BOFILE",
                            "OFILE", "MEMLOCK", "AS",    NULL};

// find the numeric value for a rlimit name
int rlimit_by_name(const char *name) {
  const size_t len = strlen(name);
  std::unique_ptr<char[]> upper_name(new char[len + 1]);
  for (size_t i = 0; i < len; i++) {
    upper_name.get()[i] = toupper(name[i]);
  }

  size_t off = 0;
  while (true) {
    const char *s = (const char *)*(tbl + off);
    if (s == NULL) {
      return -1;
    }
    if (strcmp(upper_name.get(), s) == 0) {
      return (int)off;
    }
    off++;
  }
}

void print_rlimits(void) {
  size_t off = 0;
  while (true) {
    const char *s = (const char *)*(tbl + off);
    if (s == NULL) {
      break;
    }
    LOG(INFO) << s << " (" << off << ")";
    off++;
  }
}

void read_rlimit(pid_t pid, unsigned long where, struct rlimit *rlim) {
  const size_t sz = sizeof(struct rlimit) / sizeof(long);
  for (size_t i = 0; i < sz; i++) {
    errno = 0;
    long word = ptrace(PTRACE_PEEKTEXT, pid, where + i * sizeof(long), 0);
    LOG(INFO) << "tried to peed for pid " << pid << " at "
              << (void *)(where + i * sizeof(long));
    if (word == -1 && errno) {
      perror("ptrace(PTRACE_PEEKTEXT, ...)");
      exit(1);
    }
    LOG(INFO) << "poking data to " << (long *)(rlim) + i;
    memcpy((long *)(rlim) + i, &word, sizeof(word));
  }
}

void poke_rlimit(pid_t pid, unsigned long where, struct rlimit *rlim) {
  const size_t sz = sizeof(struct rlimit) / sizeof(long);
  for (size_t i = 0; i < sz; i++) {
    long word;
    memcpy(&word, (long *)(rlim) + i, sizeof(word));
    LOG(INFO) << "tried to poke " << word << " at "
              << (void *)(where + i * sizeof(long));
    if (ptrace(PTRACE_POKETEXT, pid, where + i * sizeof(long), word)) {
      perror("ptrace(PTRACE_POKETEXT...)");
    }
  }
}
