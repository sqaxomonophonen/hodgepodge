#!/usr/bin/env bash
set -e
cc -O0 perf.c -o perf
./perf $1
