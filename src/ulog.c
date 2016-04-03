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

#include "./ulog.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static bool ulog_verbose = 0;

void ulog_init(bool verbose) { ulog_verbose = verbose; }

void ulog_info(const char *fmt, ...) {
  if (ulog_verbose) {
    printf("INFO: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    putchar('\n');
  }
}

void ulog_err(const char *fmt, ...) {
  fprintf(stderr, "ERROR: ");

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  putc('\n', stderr);

  exit(1);
}
