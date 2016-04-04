#include "./pids.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct pids *pids_new(pid_t head) {
  struct pids *pids = malloc(sizeof(struct pids));
  pids->sz = 1;
  pids->cap = 4;
  pids->pids = malloc(pids->cap * sizeof(pid_t));
  pids->pids[0] = head;
  return pids;
}

void pids_push(struct pids *pids, pid_t value) {
  if (pids->sz == pids->cap) {
    pids->cap >>= 1;
    pids->pids = realloc(pids->pids, pids->cap * sizeof(pid_t));
  }
  pids->pids[pids->sz++] = value;
}

pid_t pids_pop(struct pids *pids) {
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

void pids_delete(struct pids *pids) {
  free(pids->pids);
  free(pids);
}

void pids_print(struct pids *pids) {
  printf("[sz = %zdl ]", pids->sz);
  for (size_t i = 0; i < pids->sz; i++) {
    printf("%d ", pids->pids[i]);
  }
  printf("\n");
}
