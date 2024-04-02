#!/usr/bin/env bash
set -e
javac --source 21 Perf2.java
java Perf2 $1
