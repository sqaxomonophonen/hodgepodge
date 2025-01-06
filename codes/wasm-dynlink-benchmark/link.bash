#!/usr/bin/env bash
set -e
for t in t0foo t0bar ; do
	clang19 \
	-O2 \
	-Wall \
	-std=c11 \
	--target=wasm32 \
	-nostdlib \
	-c \
	${t}.c
done
wasm-ld --no-entry --import-memory --export-dynamic *.o -o t0.wasm

# experiment in creating a single t0.wasm from t0foo.c and t0bar.c
# perhaps compare the other .wasm files with *.o to see what makes them
# "linkable"?

