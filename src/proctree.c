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

bool pids_empty(struct pids* pids) { return (pids->sz == 0); }

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

  char* line = NULL;
  size_t len = 0;

  ulog_info("hi %d", targets->sz);
  while (!pids_empty(targets)) {
    struct dirent ent;
    struct dirent* result;

    const pid_t target = pids_pop(targets);
    ulog_info("target is %d", target);

    readdir_r(proc, &ent, &result);
    if (result == NULL) {
      ulog_info("failed to readdir_r");
      break;
    }

    ulog_info("name = %s, type = %d", ent.d_name, ent.d_type);

    if (ent.d_type == DT_DIR) {
      ulog_info("is dir");
      bool all_num = true;
      for (size_t i = 0; i < strlen(ent.d_name); i++) {
        if (!isdigit(ent.d_name[i])) {
          all_num = false;
          break;
        }
      }
      if (!all_num) {
        continue;
      }
    }

    long pidl = strtol(ent.d_name, NULL, 10);
    assert(pidl >= 0);
    assert(pidl < LONG_MAX);
    ulog_info("pidl = %ld\n", pidl);

    pids_push(targets, (pid_t)target);

    char fname[128];
    fname[0] = 0;
    strcat(fname, "/proc/");
    strcat(fname, ent.d_name);
    strcat(fname, "/status");

    FILE* stream = fopen(fname, "r");
    if (stream == NULL) {
      continue;
    }

    while (getline(&line, &len, stream) != -1) {
      if (strstr(line, "PPid:") == line) {
        puts(line);
      }
    }

    // redo it again
    rewinddir(proc);
  }

  free(line);
  closedir(proc);
  return 0;
}
