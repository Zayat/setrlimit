This is a GDB script that will increase the soft rlimit for a process to it's
max value; by default it uses `RLIMIT_CORE`.

In imaginary use case for a script like this. Imagine that for some reason you
have unlimited ulimits, but some program has put in place a soft core limit
arbitrarily. You could go modify the source code to the bad program and figure
out how to restart it on all your machines, or you can use this script.
