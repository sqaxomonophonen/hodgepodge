#!/usr/bin/env bash
set -e
gnatmake -g tasking
strace --follow-forks -o tasking.strace ./tasking
