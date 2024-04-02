#!/usr/bin/env bash
set -e
cc -O3 perf.c -o perf
./perf $1
