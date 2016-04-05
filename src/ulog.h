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

void ulog_init(int min_level);

int ulog_get_level(void);
void ulog_set_level(int level);

void ulog_debug(const char *fmt, ...);
void ulog_info(const char *fmt, ...);
void ulog_warn(const char *fmt, ...);
void ulog_err(const char *fmt, ...);

// actually terminates the program
void ulog_fatal(const char *fmt, ...);
