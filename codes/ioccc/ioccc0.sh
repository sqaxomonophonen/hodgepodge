#!/usr/bin/env bash
set -e
cc -O3 -Wno-string-plus-int ioccc0.c -o ioccc0 -lm
NO=2119
./ioccc0 $NO
echo OK
ls -l song$NO.wav ioccc0.c
echo -n "iocccsize: "
/home/aks/git/ioccc-src/mkiocccentry/iocccsize ioccc0.c || true
echo "(limits are 4993/2503)"
#hexdump -C song$NO.wav | head
#mplayer song$NO.wav
sox song$NO.wav -r 48000 song$NO.48.wav
mplayer song$NO.48.wav
