#!/bin/sh
set -e
EXE=example
rm -f $EXE
vasmm68k_mot -kick1hunks -Fhunkexe -o $EXE -nosym source.asm
DISK=example.adf
rm -f $DISK
xdftool $DISK create
xdftool $DISK format EXAMPLE
xdftool $DISK boot install
xdftool $DISK write $EXE
xdftool $DISK makedir s
echo $EXE > startup-sequence
xdftool $DISK write startup-sequence s/startup-sequence
