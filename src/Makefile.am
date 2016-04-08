AM_CXXFLAGS = -std=c++0x -Wall -Wextra -Wunused -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE

ENFORCE = enforce.cc
PROCTREE = proctree.cc
RLIM = rlim.cc
TOLONG = tolong.cc
PIDS = pids.cc

bin_PROGRAMS = setrlimit

setrlimit_SOURCES = main.cc $(RLIM) $(ENFORCE) $(PROCTREE) $(TOLONG) $(PIDS)
setrlimit_LDFLAGS = $(GLOG_LIBS)

noinst_HEADERS = enforce.h pids.h proctree.h rlim.h tolong.h
