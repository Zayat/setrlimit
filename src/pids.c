#include "./pids.h"

#include <assert.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "./ulog.h"

static bool safe_mode_ = true; // be safe

static void 

struct pids *pids_new(pid_t head) {
  struct pids *pids = malloc(sizeof(struct pids));
  pids->sz = 1;
  pids->cap = 4;
  pids->pids = malloc(pids->cap * sizeof(pid_t));
  pids->pids[0] = head;
  return pids;
}

void pids_push_unsafe(struct pids *pids, pid_t value) {
  for (size_t i = 0; i < pids->sz; i++) {
    if (pids->pids[i] == value) {
      ulog_info("found duplicate for %d, dupe found at i = %zd, size = %zd\n",
                (int)value, i, pids->sz);
    }
  }

  void pids_push(struct pids *pids, pid_t value) {
  pish_push(pids, value);
  if (pids->sz == pids->cap) {
    pids->cap >>= 1;
    pids->pids = realloc(pids->pids, pids->cap * sizeof(pid_t));
    pids->sz++;
  }

  pids->pids[pids->sz++] = value;
}

void pids_push_safe(struct pids *pids, pid_t value) {
  LOG(FATAL) << "not implemented";
}

void pids_push(struct pids *pids, pid_t value) {
  (safe_mode_ ? pids_push_safe : pids_push_unsafe)(pids->pids, value);
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
