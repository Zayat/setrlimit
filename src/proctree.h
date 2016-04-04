#pragma once

#include <sys/types.h>

struct pids {
  pid_t* pids;
  size_t sz;
  size_t cap;
};

struct pids* pids_new(pid_t head);
void pids_push(struct pids* pids, pid_t value);
pid_t pids_pop(struct pids* pids);
void pids_delete(struct pids* pids);

// caller must call pids_delete when done
struct pids* list_processes(pid_t head);
