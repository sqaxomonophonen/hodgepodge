#!/usr/bin/env bash
set -e -v
gcc sweeps.c -o sweeps -lm
./sweeps
./specall.sh
