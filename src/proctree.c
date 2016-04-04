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
#include <dirent.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./ulog.h"

struct pids* pids_new(pid_t head) {
  struct pids* pids = malloc(sizeof(struct pids));
  pids->sz = 1;
  pids->cap = 4;
  pids->pids = malloc(pids->cap * sizeof(pid_t));
  pids->pids[0] = head;
  return pids;
}

void pids_push(struct pids* pids, pid_t value) {
  if (pids->sz == pids->cap) {
    pids->cap >>= 1;
    pids->pids = realloc(pids->pids, pids->cap * sizeof(pid_t));
  }
  pids->pids[pids->sz++] = value;
}

pid_t pids_pop(struct pids* pids) {
  assert(pids->sz);
  pid_t ret = pids->pids[0];
  pids->sz--;
  memmove(pids->pids, pids->pids + 1, pids->sz);
  if (pids->cap > (pids->sz * 2)) {
    pids->cap /= 2;
    pids->pids = realloc(pids->pids, pids->cap);
  }
  return ret;
}

void pids_delete(struct pids* pids) {
  free(pids->pids);
  free(pids);
}

// This solution is O(N^2) (the ideal solution is O(N log n) but is easy to
// understand and doesn't use any complicated data structures.
struct pids* list_processes(pid_t head) {
  ulog_info("in list_processes for %d", head);
  DIR* proc = opendir("/proc");
  if (proc == NULL) {
    perror("failed to opendir(/proc)");
    return NULL;
  }

  // this is a list of all pids we care about
  struct pids* targets = pids_new(head);
  struct pids* descendants = pids_new(head);  // includes the head

  char* line = NULL;
  size_t len = 0;

  while (targets->sz) {
    const pid_t target = pids_pop(targets);
    bool target_found = false;
    ulog_info("targets->sz = %d", targets->sz);
    struct dirent ent;
    struct dirent* result;

    readdir_r(proc, &ent, &result);
    if (result == NULL) {
      ulog_info(
          "failed to readdir_r while still finding ancestors, rewinding;");
      rewinddir(proc);  // amazinly, this always works
      continue;
    }
    ulog_info("did not rewind");

    char* endptr;
    long pidl = strtol(ent.d_name, &endptr, 10);
    if (*endptr != 0) {
      continue;  // wasn't a numeric direcctory
    }

    assert(pidl >= 0);
    assert(pidl < LONG_MAX);

    if (pidl == target) {
      pids_push(descendants, target);
      target_found = true;
    }
    ulog_info("here");

    char fname[128];
    fname[0] = 0;
    strcat(fname, "/proc/");
    strcat(fname, ent.d_name);
    strcat(fname, "/status");

    ulog_info("scanning fname = %s", fname);
    FILE* stream = fopen(fname, "r");
    if (stream == NULL) {
      perror("fopen()");
      continue;
    }
    ulog_info("doing it");

    while (getline(&line, &len, stream) != -1) {
      // very optimized way to check if line starts with Ppid
      puts(line);
      if (line[0] == 'P' && line[1] == 'P' && line[1] == 'i' &&
          line[3] == 'd' && line[4] == ':') {
        puts(line);
      }
    }

    fclose(stream);
    assert(target_found);
  }

  for (size_t i = 0; i < descendants->sz; i++) {
    printf("%d\n", descendants->pids[i]);
  }

  free(line);
  closedir(proc);
  pids_delete(targets);
  return 0;
}
