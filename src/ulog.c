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
// alo.If not, see < http : // www.gnu.org/licenses/>.

#include "./ulog.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ulog_level = 10;

void ulog_init(int min_level) { ulog_level = min_level; }

#define INFO_LVL(x, min_level)          \
  void ulog_##x(const char *fmt, ...) { \
    if (min_level >= ulog_level) {      \
      printf(#x);                       \
      printf(": ");                     \
      va_list args;                     \
      va_start(args, fmt);              \
      vfprintf(stdout, fmt, args);      \
      putchar('\n');                    \
      va_end(args);                     \
      fflush(stdout);                   \
    }                                   \
  }

INFO_LVL(debug, 0)

INFO_LVL(info, 10)

INFO_LVL(warn, 20)

INFO_LVL(err, 30)

int ulog_get_level() { return ulog_level; }
void ulog_set_level(int level) { ulog_level = level; }

void ulog_fatal(const char *fmt, ...) {
  fprintf(stderr, "ERROR: ");

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  putc('\n', stderr);
  fflush(stderr);

  exit(1);
}
