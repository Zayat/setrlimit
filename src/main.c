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
#include <sys/resource.h>

#ifdef HAVE_CONFIG_H
#include "./config.h"
#endif

#include "./enforce.h"
#include "./proctree.h"
#include "./ulog.h"
#include "./rlim.h"
#include "./tolong.h"

int main(int argc, char **argv) {
  int c;
  int resource = -1;
  bool verbose = false;
  bool do_list = false;
  bool recursive = false;
  opterr = 0;
  char *resource_str = NULL;

  while ((c = getopt(argc, argv, "hlr:RvV")) != -1) {
    switch (c) {
    case 'h':
      goto usage;
      break;
    case 'l':
      do_list = true;
      break;
    case 'r':
      resource_str = optarg; // not a copy!
      break;
    case 'R':
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
      if (isprint(optopt)) {
        ulog_err("unexpectedly got option %c", optopt);
      } else {
        ulog_err("unknown option character `\\x%x'", optopt);
      }
      break;
    }
  }

  ulog_init(verbose ? 10 : 0);

  if (do_list) {
    print_rlimits();
    return 0;
  }

  const size_t arg_delta = argc - optind;
  if (arg_delta <= 0) {
    ulog_err("you didn't specify any processes to setrlimit");
    return 1;
  } else if (arg_delta >= 2) {
    ulog_err("you can only setrlimit for on process (but you can use -R for "
             "recursive action");
    return 1;
  }

  pid_t target_pid = (pid_t)ToLong(argv[optind]);
  struct pids *pids = pids_new(target_pid);
  for (int i = optind + 1; i < argc; i++) {
    pids_push(pids, ToLong(argv[1]));
  }

  if (recursive) {
    ulog_info("in recursive branch");
    add_processes_recursively(pids);
  } else {
    ulog_info("not in recursive branch");
  }

  for (size_t i = 0; i < pids->sz; i++) {
    printf("i = %zd, pid = %d", i, pids->pids[i]);
  }

  for (int i = optind; i < argc; i++) {
    ulog_info("i = %d, opdind = %d, argc = %d\n");
    printf("%d %s\n", optind, optarg);
    pids_push(pids, ToLong(argv[i]));
  }

  ulog_info("total pids is: %zd", pids->sz);
  for (size_t i = 0; i < pids->sz; i++) {
    ulog_info("%d/%d %d\n", i + 1, pids->sz, (int)pids->pids[i]);
  }
  ulog_info("done enumerating pids");

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

  int status = 0;
  while (optind < argc) {
    errno = 0;
    long maybe_pid = strtol(argv[optind++], NULL, 10);
    if (errno) {
      perror("failed to call strtol()");
    }
    if (maybe_pid < 1) {
      ulog_err("cowardly refusing to trace pid %ld", maybe_pid);
    }

    status |= enforce((pid_t)maybe_pid, resource);
  }

  return status;

usage:
  fprintf(stderr, "usage: %s: PID...\n", argv[0]);
  return 1;
}
