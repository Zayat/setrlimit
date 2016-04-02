#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([setrlimit], [0.1], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
