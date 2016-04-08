
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

#include <glog/logging.h>

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

size_t AddChildren(struct pids *pids) {
  LOG(INFO) << "in AddChildren";
  assert(pids->sz == 1);

  bool first = true;
  size_t s = 0, e = 1;
  size_t total = 0;
  size_t loop_no = 0;
  while (true) {
    LOG(INFO) << "in looop number " << loop_no++;
    size_t loop_total = 0;
    std::vector<pid_t> targets, children;
    LOG(INFO) << "first = " << first;
    if (first) {
      first = false;
      targets.push_back(pids->pids[0]);
    } else {
      for (size_t i = s; i < e; i++) {
        targets.push_back(pids->pids[i]);
      }
    }
    LOG(INFO) << "add_children, s = " << s << ", e = " << e
              << ", target.size() = " << targets.size();
    for (const auto &t : targets) {
      LOG(INFO) << "in a targets loop of " << targets.size();
      std::stringstream ss;
      ss << "/proc/" << t << "/task/";
      const std::string task_line = ss.str();

      DIR *task_dir = opendir(task_line.c_str());
      if (task_dir == nullptr) {
        std::cerr << "non-fatal opendir of " << task_line << ": "
                  << strerror(errno) << "\n";
        continue;
      }

      long val;
      struct dirent ent;
      struct dirent *rslt;

      while (true) {
        assert(task_dir != nullptr);
        const int ret = readdir_r(task_dir, &ent, &rslt);
        if (ret || rslt == NULL) {
          LOG(INFO) << "finished reading directory";
          break;
        }

        errno = 0;
        char *endptr;
        val = strtol(ent.d_name, &endptr, 10);
        if ((errno == ERANGE && (val == LONG_MIN || val == LONG_MAX)) ||
            (errno != 0 && val == 0)) {
          LOG(INFO) << "encountered not a pid";
          continue;  // this is not a pid
        }
        if (*endptr != '\0') {
          continue;
        }

        if (val <= 0) {
          LOG(INFO) << ent.d_name << " cowardly refusing to handle pid " << val;
          continue;
        }
        LOG(INFO) << "found child " << val;

        pids_push(pids, val);

        std::stringstream sss;
        sss << task_line << val << "/children";
        const std::string s_copy = sss.str();
        LOG(INFO) << "trying to open " << s_copy;
        std::ifstream children_file(s_copy);
        if (!children_file.good()) {
          LOG(WARNING) << "children file not good";
          continue;
        }

        std::string line;
        std::getline(children_file, line);
        LOG(INFO) << "line sz is " << line.size();

        std::stringstream lss(line);
        std::vector<int> vchild{std::istream_iterator<int>(lss),
                                std::istream_iterator<int>()};

        loop_total += vchild.size();

        LOG(INFO) << "vchild_size " << vchild.size();

        for (size_t i = 0; i < vchild.size(); i++) {
          LOG(INFO) << "i = " << i << ", etnry is pid " << vchild[i];
          pids_push(pids, vchild[i]);
        }
      }
      LOG(INFO) << "about to call closedir, loop_total = " << loop_total;

      assert(task_dir != NULL);
      closedir(task_dir);
    }

    LOG(INFO) << targets.size()
              << " targets finished, loop_total = " << loop_total;

    if (loop_total) {
      s = e;
      e += loop_total;
      total += loop_total;
      LOG(INFO) << "total = " << total << ", loop_total = " << loop_total
                << ", sz = " << pids->sz << ", s = " << s << ", e = " << e;
    } else {
      break;
    }
  }
  return 0;
}
