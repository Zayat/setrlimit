#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([setrlimit], [1.0], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([src/main.cc])

AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES(GFLAGS, libgflags)
PKG_CHECK_MODULES(GLOG, libglog)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
