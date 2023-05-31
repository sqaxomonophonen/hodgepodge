#!/usr/bin/env bash
time zig build -freference-trace && ./zig-out/bin/zig
