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

// this could be way more optimized
static bool starts_with(const char *needle, const char *haystack) {
  if (!(needle || haystack)) {
    return false;
  }
  const size_t ns = strlen(needle);
  if (strlen(haystack) < ns) {
    return false;
  }
  for (size_t i = 0; i < ns; i++) {
    if (haystack[i] != needle[i]) {
      return false;
    }
  }
  return true;
}

int add_children(struct pids *pids) {
  ulog_info("in add_children");
  assert(pids->sz == 1);

  DIR *proc = opendir("/proc");
  if (proc == NULL) {
    perror("opendir");
    return -1;
  }
  int found = 0;
  size_t len = 0;
  char *line = NULL;
  pid_t target_pid = pids->pids[0];

  for(;;) {
    struct dirent ent;
    struct dirent *rslt;
    size_t found = 0;

    if (readdir_r(proc, &ent, &rslt)) {
      if (found == 0) {
        break;
      }
      rewinddir(proc);
    }

    ulog_info("ent.d_name = %s", ent.d_name);

    char *endptr;
    long val = strtol(ent.d_name, &endptr, 10);
    if (ent.d_name != NULL && *endptr == '\0') {
      ulog_info("this is a pid: %d", pids);
    }
    ulog_info("ent.d_name = %s, our version is %d", ent.d_name, val);

    static char status_buf[256];
    assert(snprintf(status_buf, sizeof(status_buf), "/proc/%ld/status", val) > 0);

    ssize_t read;
    FILE *status = fopen(status_buf, "r");
    if (status == NULL) {
      perror("fopen()");
      continue;
    }
    while ((read = getline(&line, &len, status))) {
      if (starts_with("Ppid:", line)) {
        printf("%d, %s", target_pid, line);
      }
    }
  }
  ulog_info("found = %zd\n", found);
  ulog_info("leaving add_children");
  return found;
}
