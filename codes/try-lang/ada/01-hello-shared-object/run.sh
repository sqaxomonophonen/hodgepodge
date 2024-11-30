#!/usr/bin/env bash
set -e
gnatgcc -c -fPIC hello.adb
gnatbind -n hello.ali
gnatlink hello.ali -shared -o hello.so
cc -Wall test_so.c -o test_so
./test_so
