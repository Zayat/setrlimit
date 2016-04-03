# Setrlimit

Increase the soft rlimit for a process to its max rlimit.

## Compiling

From git:

    ./autogen.sh  # make sure you have autoconf installed
    ./configure
    make

Make a source tarball:

    ./configure
    make

## Usage

Use it like this

    setrlimit <pid>

The default behavior if to change the value for `RLIMIT_CORE`, but you can
change other limits for a process as well. For instance, you could do:

    serlimit -r nfiles <pid>

And this would increase the `RLIMIT_NFILES` soft limit for pid.

## Portability

This has only been tested on 64-bit Linux. It probably won't work on 32-bit
Linux without changes.

## How Safe Is This?

This code makes use of the [Intel x86-64](https://eklitzke.org/red-zone]. This
is an esoteric feature and not widely used. However if you attach to a process
that is currently using the red zone then this could corrupt its red zone stack
contents.

In practice this issue has not been observed. **tl;dt** this is safe.

## Background

There is this really weird ancient thing on Unix called rlimits. It's one of
those things that everyone agrees isn't really a great pr of Unix (there are
modern, better designs, but we're stuck with it). It's extra confusing because
there are both kernel/system default rlimits as well as rlimits imposed by bash
(which is frequently used to launch programs).

The basic idea is that there are a certain number of limits, things like max
address space or max number of file locks, that can be imposed on a process.
There is a soft limit and a hard limit.

The hard limit can be decreased but not increased. The soft limit can be
decreased or increased, but cannot be increased above the current hard limit.

For an arbitrary process, the best way to look at its limits are using:

    cat /proc/PID/limits

A program hits a limit when it hits the soft limit (which may also be its hard
limit). At this point the process will get various errors when trying to
allocate things like more file descriptors or more memory.

For some reason unclear to me, in the byzantine tech stack at work there's a
process that's calls `setrlimit(2)` to set the soft limit value for
`RLIMIT_CORE` to 0 but doesn't change the hard limit (which is infinite). I
don't know what's doing this, if there's a setting to disable it, etc. That part
is under investigation. But that process goes on to be the ancestor parent of a
bunch of children, and the children inherit the soft limit of 0 effectively
meaning that they never dump core.

A lot of people don't need core dumps, but for my work it was essential. My
original solution was to use GDB to attach to a process and increase the limits.
That got cumbersome so that was turned into a set of GDB scripts that turned
into this C program.

## Copyright / Licensing

Notices are in all of the software files, but the tl;dr is this project is
released under the terms of the GPL v3 (or later).
