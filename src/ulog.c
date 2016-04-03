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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ulog_verbose = 0;

// this does something naughty and trims trailing whitespace (esp. newlines form
// format strings)
inline bool trim_fmt(const char *fmt) {
  bool didit = false;
  const size_t sz = strlen(fmt);
  char *mut_fmt = (char *)fmt;

  size_t i = sz - 1;
  while (true) {
    if (isspace(mut_fmt[i])) {
      mut_fmt[i] = '\0';
      didit = true;
    }
    if (i == 0) {
      break;
    }
    i--;
  }
  return didit;
}

void ulog_init(bool verbose) { ulog_verbose = verbose; }

void ulog_info(const char *fmt, ...) {
  if (ulog_verbose) {
    printf("INFO: ");

    va_list args;
    va_start(args, fmt);
    trim_fmt(fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    putchar('\n');
  }
}

void ulog_err(const char *fmt, ...) {
  fprintf(stderr, "ERROR: ");

  va_list args;
  va_start(args, fmt);
  trim_fmt(fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  putc('\n', stderr);

  exit(1);
}
