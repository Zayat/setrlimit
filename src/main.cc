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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#ifdef HAVE_CONFIG_H
#include "./config.h"
#endif

#include "./enforce.h"
#include "./pids.h"
#include "./proctree.h"
#include "./rlim.h"
#include "./tolong.h"

DEFINE_int32(resource, RLIMIT_CORE, "the resource to limit");
DEFINE_bool(recursive, false, "whether to search recursively");
DEFINE_bool(list, false, "list rlimits");

static inline void usage(const char *prog, int status = EXIT_FAILURE) {
  fprintf(stderr, "usage: %s: [-v] [-r] [-R resource] PID...\n", prog);
  exit(status);
}

int main(int argc, char **argv) {
  google::SetUsageMessage("[OPTIONS] PID");
#ifdef HAVE_CONFIG_H
  google::SetVersionString(VERSION);
#endif
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "entered main";

  if (FLAGS_list) {
    print_rlimits();
    return 0;
  }

  if (argc == 0) {
    LOG(ERROR) << "usage: setrlimit [-v] [-recurisve] PID...";
    return 1;
  }

  LOG_IF(FATAL, argc <= 1) << "you must specify some pids to setrlimit";
  struct pids *pids = pids_blank();
  for (int i = 1; i < argc; i++) {
    pids_push(pids, ToLong(argv[i]));
  }
  CHECK(pids->sz);

  if (FLAGS_list) {
    pids_print(pids);
  }

  return 0;
  if (FLAGS_recursive) {
    LOG(INFO) << "recursively apply limits to descendants";
    AddChildren(pids);
  }

  LOG(INFO) << "total process size is " << pids->sz;
  for (size_t i = 0; i < pids->sz; i++) {
    LOG(INFO) << (i + 1) << " of " << pids->sz << ", setting pid "
              << pids->pids[i];
  }

  int resource = RLIMIT_CORE;

  LOG(INFO) << "final value for resource is: " << resource;

  int status = 0;
  while (pids->sz) {
    LOG(INFO) << "sz = " << pids->sz;
    const pid_t target = pids_pop(pids, NULL);
    LOG(INFO) << "pids = " << target;
    status |= enforce(target, resource);
  }
  if (status && geteuid() != 0) {
    LOG(ERROR) << "some processes failed, may want to retry as root";
  }

  for (size_t i = 0; i < pids->sz; i++) {
    LOG(INFO) << "acted on pid " << pids->pids[i];
  }

  LOG(INFO) << "exiting with status " << status;
  return status;
}
