#!/usr/bin/env bash
set -e
for t in t0foo t0bar t1 t2foo t2bar ; do
	clang19 \
	-O2 \
	-Wall \
	-std=c11 \
	--target=wasm32 \
	-nostdlib \
	-Wl,--no-entry \
	-Wl,--import-memory \
	-Wl,--export-dynamic \
	-Wl,--unresolved-symbols=import-dynamic \
	-o ${t}.wasm \
	${t}.c
done
