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

#include "./tolong.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

long ToLong(const char *s) {
  char *endptr;
  errno = 0;
  const long val = strtol(s, &endptr, 10);

  if (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) {
    perror("strtol: input was outside long range");
    exit(EXIT_FAILURE);
  }
  if (errno == EINVAL && val == 0) {
    perror("strtol: ernno");
    exit(EXIT_FAILURE);
  }
#if 0
  if (*endptr == *s) {
    fprintf(stderr, "No digits were found\n");
    exit(EXIT_FAILURE);
  }
#endif
  return val;
}
