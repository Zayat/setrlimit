AM_CFLAGS = -std=c99 -Wall -Wextra -Wunused -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE

ENFORCE = enforce.c
PROCTREE = proctree.c
ULOG = ulog.c
RLIM = rlim.c
TOLONG = tolong.c

bin_PROGRAMS = setrlimit

setrlimit_SOURCES = main.c $(ULOG) $(RLIM) $(ENFORCE) $(PROCTREE) $(TOLONG)

noinst_HEADERS = enforce.h proctree.h rlim.h tolong.h ulog.h
