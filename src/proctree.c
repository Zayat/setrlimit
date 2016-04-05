// Copyright Evan Klitzke <evan@eklitzke.org>, 2016X)
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

#include "./tolong.h"
#include "./ulog.h"

// spells out "PPid: "
#define PPID_PREFIX 0x507069643a000000

void add_processes_recursively(struct pids *pids) {
  ulog_init(10);
  ulog_info("entered add_processes_recursively");
  assert(pids->sz == 1);  // can really be > 1, but we just handle 1 for now

  DIR *proc = opendir("/proc");
  if (proc == NULL) {
    perror("failed to opendir(/proc)");
    return;
  }

  long pidl;
  char *line = NULL;
  size_t len = 0;

  bool found_any = true;
  while (found_any) {
    found_any = false;
    struct dirent ent;
    struct dirent *result;

    readdir_r(proc, &ent, &result);
    if (result == NULL) {
      ulog_info(
          "failed to readdir_r while still finding ancestors, rewinding;");
      rewinddir(proc);  // amazinly, this always works
      continue;
    }

    char *endptr;
    pidl = strtol(ent.d_name, &endptr, 10);
    if (*endptr != 0) {
      continue;  // wasn't a numeric direcctory
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

    // can grow
    size_t pid_capacity = 16;

    const char *pid_name = malloc(pid_capacity);
    size_t start = 0, end = 0;

    bool keep_going = true;
    ssize_t pos;
    ulog_info("opening %s looking for ppid", fname);
    while (keep_going && (pos = getline(&line, &len, f) != -1)) {
      long val;
      memcpy(&val, line, sizeof(val));
      // very optimized way to check if line starts with Ppid
      if ((val & PPID_PREFIX) == PPID_PREFIX) {
        ulog_info("full line is: %s", line);

        for (size_t i = 0; i < len; i++) {
          ulog_info("i = %d, len = %zd", i, len);
          if (!start && isdigit(line[i])) {
            start = line[i];
          } else if (start && line[i] == '\0') {
            end = line[i];
            break;
          }
        }

        // we may need to allocate more capacity and retry
        if (!(start || end)) {
          pid_capacity *= 2;
          pid_name = realloc((struct rlimits *)pid_name, pid_capacity);
          keep_going = false;
        }
        const char *ppid_s = (char *)strdup((const char *)(end - start + 1));
        ulog_info("ppid = %s, %s, ppid_s = %s", pid_name, ppid_s);
        pidl = ToLong(ppid_s);
        ulog_info("pidl = %ld", pidl);
        found_any = true;
      }
    }
    ulog_info("doing reading from file");
    ulog_info("pidl = %ld", pidl);

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
    pritnf("fond_any = %d\n", found_any);
  }

  for (size_t i = 0; i < descendants->sz; i++) {
    printf("%d\n", descendants->pids[i]);
  }
#endif

    // cleanup:
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
