#include "./pids.h"

#include <assert.h>
#include <glog/logging.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SZ 4

struct pids *pids_new(pid_t head) {
  struct pids *pids = (struct pids *)malloc(sizeof(struct pids) * DEFAULT_SZ);
  pids->sz = 1;
  pids->cap = DEFAULT_SZ;
  pids->pids = (pid_t *)malloc(pids->cap * sizeof(pid_t));
  pids->pids[0] = head;
  return pids;
}

bool pids_empty(struct pids *pids) { return pids->sz > 0; }

size_t pids_push(struct pids *pids, pid_t value) {
  bool found = false;
  for (size_t i = 0; i < pids->sz; i++) {
    if (pids->pids[i] == value) {
      VLOG(1) << "found duplicate for " << value << ", dupe found at i = " << i
              << ", size = " << pids->sz;
      found = true;
    }
  }
  if (!found) {
    pids->pids[pids->sz++] = value;
  }
  return pids->sz;
}

pid_t pids_pop(struct pids *pids, size_t *size) {
  assert(pids->sz);
  pid_t ret = pids->pids[0];
  pids->sz--;
  memmove(pids->pids, pids->pids + 1, pids->sz);
  if (pids->cap > (pids->sz * 2)) {
    pids->cap /= 2;
    pids->pids = (pid_t *)realloc(pids->pids, pids->cap);
  }
  if (size != NULL) {
    *size = pids->sz;
  }
  return ret;
}

void pids_delete(struct pids *pids) {
  free(pids->pids);
  free(pids);
}

void pids_print(struct pids *pids) {
  printf("sz = %zd:", pids->sz);
  for (size_t i = 0; i < pids->sz; i++) {
    printf(" %d", pids->pids[i]);
  }
  printf("\n");
}
