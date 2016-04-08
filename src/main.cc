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
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

#include <glog/logging.h>

#ifdef HAVE_CONFIG_H
#include "./config.h"
#endif

#include "./enforce.h"
#include "./pids.h"
#include "./proctree.h"
#include "./rlim.h"
#include "./tolong.h"

static inline void usage(const char *prog, int status = EXIT_FAILURE) {
  fprintf(stderr, "usage: %s: [-v] [-r] [-R resource] PID...\n", prog);
  exit(status);
}

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);

  int c;
  int resource = -1;
  bool verbose = false;
  bool do_list = false;
  bool recursive = false;
  opterr = 0;
  char *resource_str = NULL;

  while ((c = getopt(argc, argv, "hlrR:vV")) != -1) {
    switch (c) {
      case 'h':
        usage(argv[0]);
        break;
      case 'l':
        do_list = true;
        break;
      case 'R':
        resource_str = optarg;  // not a copy!
        break;
      case 'r':
        recursive = true;
        break;
      case 'v':
        verbose = true;
        break;
      case 'V':
        puts(PACKAGE_STRING);
        return 0;
        break;
      case '?':
        LOG(FATAL) << "unexpectedly got option %c" << optopt;
        break;
    }
  }

  if (do_list) {
    print_rlimits();
    return 0;
  }

  const size_t arg_delta = argc - optind;
  if (arg_delta <= 0) {
    LOG(FATAL) << "you didn't specify any processes to setrlimit";
    return 1;
  } else if (arg_delta >= 2) {
    LOG(FATAL)
        << "you can only setrlimit for on process (but you can use -R for "
           "recursive action";
    return 1;
  }

  pid_t target_pid = (pid_t)ToLong(argv[optind]);
  struct pids *pids = pids_new(target_pid);
  for (int i = optind + 1; i < argc; i++) {
    pids_push(pids, ToLong(argv[i]));
  }

  if (verbose) {
    pids_print(pids);
  }

  if (recursive) {
    LOG(INFO) << "recursively apply limits to descendants";
    AddChildren(pids);
  }

  for (int i = optind + 1; i < argc; i++) {
    pids_push(pids, ToLong(argv[i]));
  }

  LOG(INFO) << "total process size is " << pids->sz;
  for (size_t i = 0; i < pids->sz; i++) {
    LOG(INFO) << (i + 1) << " of " << pids->sz << ", setting pid "
              << pids->pids[i];
  }
  LOG(INFO) << "done enumerating pids sz = " << pids->sz
            << ", looking for descendants";

  if (resource_str != NULL) {
    resource = rlimit_by_name(resource_str);
    if (resource == -1) {
      errno = 0;
      resource = strtol(argv[optind], NULL, 10);
    }
  }

  if (resource < 0) {
    LOG(INFO) << "reluctantly setting resource to RLIMIT_CORE";
    resource = RLIMIT_CORE;
  }

  LOG(INFO) << "final value for resource is" << resource;

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
