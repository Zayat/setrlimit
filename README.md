## What Is This?

This is a C script that will use the `ptrace(2)` system call to take control of
a process and then invoke `getrlimit(2)` and `setrlimit(2)` on that process'
behalf, so that the limits are as high as possible.

## Compiling/Installing

If you got this from GitHub, you will need to install autotools and then run:

    ./auotogen.sh
    ./configure
    make

If you got this from a tarball:

    ./configure
    make

## How Safe Is This?

This is safe in practice, unsafe in theory.

This uses the Intel x86-64 "red zone" for its `struct rlimit`. In theory a
properly optimized compiler could be storing data there and this could cause
corruption.

If you can find a program that exhibits corruption when using this, please point
it out to me.

## Use Case

I have a system at work that works something like this:

    init
      |
    supervisord
    | | | | | |
    a million children

The children can spawn more of their own and so forth.

The default behavior on the process is that a process has a soft/hard infinite
ulimit for `RLIMIT_CORE`. Something the aforementioned stack (includind the
bundle of joy that is uwsgi) is using `setrlimit(2)` to set `RLIMIT_CORE` to 0.
But only the soft limit, the hard limit is still infinite.
