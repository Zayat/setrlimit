ENFORCE = enforce.c
ULOG = ulog.c
RLIM = rlim.c

bin_PROGRAMS = setrlimit
AM_CFLAGS = -std=c11 -g -Wall -Wextra -Wunused

setrlimit_SOURCES = main.c $(ULOG) $(RLIM) $(ENFORCE)

noinst_HEADERS = enforce.h rlim.h ulog.h
