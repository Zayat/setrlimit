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
        resource_str = optarg;  // not a copy!
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

  if (argc - optind <= 0) {
    ulog_err("you didn't specify any processes to setrlimit");
    return 1;
  }

  struct pids *pids = pids_new(ToLong(argv[optind]));
  for (int i = optind; i < argc; i++) {
    errno = 0;
    long pid = strtol(argv[optind], NULL, 10);
    if (errno) {
      ulog_info("couldn't parse all args");
      return 1;
    }
    if (pid >= 2) {
      pids_push(pids, (pid_t)pid);
    }
  }

  if (recursive) {
    struct pids *rpids = list_processes(1);
    // we can skip the first pid
    for (size_t i = 1; i < rpids->sz; i++) {
      pids_push(pids, rpids->pids[i]);
      pids_delete(rpids);
    }
  }

  ulog_info("total pids is %d", pids->sz);

  for (int i = optind; i < argc; i++) {
    printf("%d %s\n", optind, optarg);
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

  list_processes(1);

  return status;

usage:
  fprintf(stderr, "usage: %s: PID...\n", argv[0]);
  return 1;
}
