#!/usr/bin/env bash
set -e
make
trials="listing_0037_single_register_mov listing_0038_many_register_mov"
for trial in $trials ; do
	echo
	echo "TRIAL [ $trial ]"
	nasm ${trial}.asm
	./disasm8086 ${trial} > __DISASM_${trial}.asm
	cat __DISASM_${trial}.asm
	nasm __DISASM_${trial}.asm
	diff -q ${trial} __DISASM_${trial} # non-zero exit code if files differs
	echo "TEST OK"
	echo
done
