#!/bin/sh
if [ $# != 1 ]; then
    echo "usage: $0 PID"
    exit 1
fi
set -eu
gdb -p $1 <gdbscript
grep 'core file' /proc/$1/limits
