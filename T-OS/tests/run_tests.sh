#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

echo "Compiling tests..."
gcc -I.. test_string16.c -o test_string16
# The scheduler suite compiles sched.c for the host (-DSCHED_HOSTTEST swaps the
# assembly context switch for a stub). No -I.. here: it must pick up the real
# system headers, not the freestanding ones T-OS ships for its own compiler.
gcc -Wall -Wextra -DSCHED_HOSTTEST ../sched.c test_sched.c -o test_sched

echo "Running tests..."
./test_string16
./test_sched

echo "Cleaning up..."
rm test_string16 test_sched
