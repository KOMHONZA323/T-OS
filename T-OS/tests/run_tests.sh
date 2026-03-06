#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "$DIR"

echo "Compiling tests..."
gcc -I.. test_string16.c -o test_string16

echo "Running tests..."
./test_string16

echo "Cleaning up..."
rm test_string16
