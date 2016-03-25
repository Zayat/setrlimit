#!/bin/sh
if [ $# != 1 ]; then
    echo "usage: $0 PID"
    exit 1
fi
exec gdb -p $1 <gdbscript
