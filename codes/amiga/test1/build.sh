#!/bin/sh
set -e
EXE=test1
rm -f $EXE
vasmm68k_mot -kick1hunks -Fhunkexe -o $EXE -nosym source.asm
echo "executable size: "
cat $EXE | wc -c
DISK=test1.adf
rm -f $DISK
xdftool $DISK create
xdftool $DISK format EXAMPLE
xdftool $DISK boot install
xdftool $DISK write $EXE
xdftool $DISK makedir s
echo $EXE > startup-sequence
xdftool $DISK write startup-sequence s/startup-sequence
rm -f startup-sequence
