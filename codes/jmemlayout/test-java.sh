#!/usr/bin/env bash
set -e
javac --enable-preview --source 21 Perf.java
java --enable-preview Perf $1
