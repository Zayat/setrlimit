AM_CXXFLAGS = -std=c++0x -Wall -Wextra -Wunused -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE

ENFORCE = enforce.cc
PROCTREE = proctree.cc
RLIM = rlim.cc
TOLONG = tolong.cc
PIDS = pids.cc

GOOG_LIBS = $(GFLAGS_LIBS) $(GLOG_LIBS)
GOOG_CFLAGS = $(GFLAGS_CFLAGS) $(GLOG_CFLAGS)

bin_PROGRAMS = setrlimit

setrlimit_SOURCES = main.cc $(RLIM) $(ENFORCE) $(PROCTREE) $(TOLONG) $(PIDS)
setrlimit_LDFLAGS = $(GOOG_LIBS)
setrlimit_CXXFLAGS = $(GOOG_CFLAGS)

noinst_HEADERS = enforce.h pids.h proctree.h rlim.h tolong.h
