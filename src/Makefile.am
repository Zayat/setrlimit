ENFORCE = enforce.c
PROCTREE = proctree.c
ULOG = ulog.c
RLIM = rlim.c

bin_PROGRAMS = setrlimit
AM_CFLAGS = -std=c11 -g -Wall -Wextra -Wunused -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE

setrlimit_SOURCES = main.c $(ULOG) $(RLIM) $(ENFORCE) $(PROCTREE)

noinst_HEADERS = enforce.h proctree.h rlim.h ulog.h
