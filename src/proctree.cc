
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
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <sstream>
#include <set>
#include <string>
#include <vector>

#include "./tolong.h"
#include "./ulog.h"

size_t add_children(struct pids *pids) {
  ulog_info("in add_children");
  assert(pids->sz == 1);

  bool first = true;
  size_t s = 0, e = 1;
  size_t total = 0;
  while (true) {
    size_t loop_total = 0;
    std::vector<pid_t> targets, children;
    ulog_info("first = %d", first);
    if (first) {
      first = false;
      targets.push_back(pids->pids[0]);
    } else {
      for (size_t i = s; i < e; i++) {
        targets.push_back(pids->pids[i]);
      }
    }
    ulog_info("add_children, s = %zd, e = %zd, targets.size() = %zd", s, e,
              targets.size());

    ulog_info("targets = %d", targets.size());

    for (const auto &t : targets) {
      ulog_info("in targets");
      std::stringstream ss;
      ss << "/proc/" << t << "/task/";
      const std::string task_line = ss.str();

      DIR *task_dir = opendir(task_line.c_str());
      if (task_dir == nullptr) {
        perror("task_dir()");
        if (errno == EPERM) {
          continue;
        }
      }

      long val;
      struct dirent ent;
      struct dirent *rslt;

      while (true) {
        const int ret = readdir_r(task_dir, &ent, &rslt);
        if (ret || rslt == NULL) {
          ulog_info("finished reading directory");
          break;
        }

        errno = 0;
        char *endptr;
        val = strtol(ent.d_name, &endptr, 10);
        if ((errno == ERANGE && (val == LONG_MIN || val == LONG_MAX)) ||
            (errno != 0 && val == 0)) {
          ulog_debug("encountered not a pid");
          continue;  // this is not a pid
        }
        if (val <= 0) {
          ulog_debug("cowardly refusing to handle pid %d", val);
          continue;
        }
        ulog_info("found child %ld", val);

        pids_push_safe(pids, val);

        std::stringstream sss;
        sss << task_line << val << "/children";
        const std::string s_copy = sss.str();
        ulog_info("trying to open %s", s_copy.c_str());
        std::ifstream children_file(s_copy);
        if (!children_file.good()) {
          ulog_info("children file not good");
          continue;
        }

        std::string line;
        std::getline(children_file, line);
        ulog_info("line sz is %zd", line.size());

        std::stringstream lss(line);
        std::vector<int> vchild{std::istream_iterator<int>(lss),
                                std::istream_iterator<int>()};

        loop_total += vchild.size();

        ulog_info("vchild_size %d", vchild.size());

        for (size_t i = 0; i < vchild.size(); i++) {
          ulog_info("i = %zd, entry is pid %d", i, vchild[i]);
          pids_push_safe(pids, vchild[i]);
        }
      }
      ulog_info("about to call closedir, looop_total = %zd", loop_total);
      closedir(task_dir);
    }

    if (loop_total) {
      s = e;
      e += loop_total;
      total += loop_total;
      ulog_info("total = %zd, loop_total = %zd, sz = %d, s = %zd, e = %zd",
                total, loop_total, pids->sz, s, e);
    } else {
      break;
    }
  }
  return 0;
}
