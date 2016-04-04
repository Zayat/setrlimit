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

#include "./proctree.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./ulog.h"

void add_processes_recursively(struct pids *pids) {
  ulog_info("entered add_processes_recursively");
  assert(pids->sz == 1); // can really be > 1, but we just handle 1 for now

  DIR *proc = opendir("/proc");
  if (proc == NULL) {
    perror("failed to opendir(/proc)");
    return;
  }

  FILE *f = NULL;
  char *line = NULL;
  size_t len = 0;

  bool found_any = true;
  while (found_any) {
    struct dirent ent;
    struct dirent *result;

    readdir_r(proc, &ent, &result);
    if (result == NULL) {
      ulog_info(
          "failed to readdir_r while still finding ancestors, rewinding;");
      rewinddir(proc); // amazinly, this always works
      continue;
    }

    char *endptr;
    long pidl = strtol(ent.d_name, &endptr, 10);
    if (*endptr != 0) {
      continue; // wasn't a numeric direcctory
    }

    assert(pidl >= 0);
    assert(pidl < LONG_MAX);

    char fname[128];
    fname[0] = 0;
    strcat(fname, "/proc/");
    strcat(fname, ent.d_name);
    strcat(fname, "/status");

    FILE *f = fopen(fname, "r");
    if (f == NULL) {
      ulog_debug("error opening %s so we are skipping it", fname);
      continue;
    }

    size_t pid_capacity = 16;
    const char *pid_name = malloc(pid_capacity);
    size_t start, end;

    ulog_info("opening %s looking for ppid", fname);
    while (getline(&line, &len, f) != -1) {
      // very optimized way to check if line starts with Ppid
      printf("line is \"%s\"\n", line);
      if (line[0] == 'P' && line[1] == 'P' && line[2] == 'i' &&
          line[3] == 'd' && line[4] == ':') {

      find_ppid:
        start = 0;
        end = 0;

        for (size_t i = 0; i < len; i++) {
          if (!start && isdigit(line + i) {
            start = line + i;
          } else if (start && *(line + i) == '\0') {
            end = line + i;
            break;
          }
        }

        // we may need to allocate more capacity and retry
        if (end == 0) {
          pid_capacity *= 2;
          pid_name = realloc(pid_name, pid_capacity);
          goto find_ppid;
        }
      }
      ulog_info("ppid = %S", pid_name);
    }

#if 0
    for (size_t i = 0; i < pids->sz; i++) {
      if (p == pids->pids[i]) {
        pids_push(pids, p);
      }
    }

    if (pidl == target) {
      pids_push(descendants, target);
      target_found = true;
    }
    ulog_info("here");

    ulog_info("scanning fname = %s", fname);
    FILE *stream = fopen(fname, "r");
    if (stream == NULL) {
      continue;
    }
    ulog_info("doing it");

    fclose(stream);
    assert(target_found);
  }

  for (size_t i = 0; i < descendants->sz; i++) {
    printf("%d\n", descendants->pids[i]);
  }
#endif

  cleanup:
    if (proc != NULL) {
      closedir(proc);
      proc = NULL;
    }

    if (line == NULL) {
      free(line);
      line = NULL;
    }

    if (f != NULL) {
      fclose(f);
      f = NULL;
    }
    return;
  }
}
