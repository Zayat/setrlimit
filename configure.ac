AC_PREREQ([2.68])
AC_INIT([statsrelay], m4_esyscmd([tr -d '\n' < VERSION]), [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.cc])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
PKG_CHECK_MODULES(GFLAGS, libgflags)
PKG_CHECK_MODULES(GLOG, libglog)

# Checks for header files.
AC_CHECK_HEADERS([limits.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
m4_ifdef([AC_CHECK_HEADERS_STDBOOL],[AC_CHECK_HEADERS_STDBOOL])
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove strerror strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_REVISION([mr_esyscmd([git describe --always])])
AC_OUTPUT
