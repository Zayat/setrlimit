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

#pragma once

#include <sys/types.h>

static_assert(sizeof(struct rlimit) > sizeof(long), "rlimit too small");
static_assert(sizeof(struct rlimit) % sizeof(long) == 0, "rlimit not aligned");

extern const size_t kNrLimit;

// This looks up an RLIMIT by name. For instance:
//
//  int_by_name("CORE") -> 4
//
// Any error will return -1
int rlimit_by_name(const char *name);

void read_rlimit(pid_t pid, unsigned long where, struct rlimit *rlim);

void poke_rlimit(pid_t pit, unsigned long where, struct rlimit *rlim);
