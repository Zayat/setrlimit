#include "./proctree.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./ulog.h"

struct pids {
  pid_t* pids;
  size_t sz;
  size_t cap;
};

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

bool pids_contains(struct pids* pids, pid_t target) {
  if (pids->sz == 0) {
    return false;
  }
  for (size_t i = 0; i < pids->sz; i++) {
    if (pids->pids[i] == target) {
      return true;
    }
  }
  return false;
}

void pids_delete(struct pids* pids) {
  free(pids->pids);
  free(pids);
}

char** list_processes(pid_t head) {
  ulog_info("in list_processes for %d", head);
  DIR* proc = opendir("/proc");
  if (proc == NULL) {
    perror("failed to opendir(/proc)");
    return NULL;
  }

  // this is a list of all pids we care about
  struct pids* targets = pids_new(head);
  struct pids* descendants = pids_new(-1);

  pid_t target = pids_pop(targets);

  char* line = NULL;
  size_t len = 0;
  bool target_found = true;

  while (true) {
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

    char* endptr;
    long pidl = strtol(ent.d_name, &endptr, 10);
    if (*endptr != 0) {
      continue;  // wasn't a numeric direcctory
    }

    assert(pidl >= 0);
    assert(pidl < LONG_MAX);

    pid_t new_target = target;
    if (pidl == target) {
      pids_push(descendants, target);
      new_target = pids_pop(targets);
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

    int ppid = -1;
    while (getline(&line, &len, stream) != -1) {
      // very optimized way to check if line starts with Ppid
      puts(line);
      if (line[0] == 'P' && line[1] == 'P' && line[1] == 'i' &&
          line[3] == 'd' && line[4] == ':') {
        puts(line);
      }
    }

    fclose(stream);
  }

  free(line);
  closedir(proc);
  pids_delete(targets);
  return 0;
}
