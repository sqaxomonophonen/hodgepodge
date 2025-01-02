#!/usr/bin/env bash
set -e
#cc -O3 -Wno-string-plus-int ioccc1.c -o ioccc1 -lm
cc -O0 -g -Wno-string-plus-int ioccc1.c -o ioccc1 -lm
ls -l ioccc1.c
echo -n "iocccsize: "
/home/aks/git/ioccc-src/mkiocccentry/iocccsize ioccc1.c || true
./ioccc1
echo OK
echo "(limits are 4993/2503)"
sox song.wav -r 48000 song.48.wav
mplayer song.48.wav

