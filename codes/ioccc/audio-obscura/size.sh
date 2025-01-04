#!/bin/sh
~/git/ioccc-src/mkiocccentry/iocccsize prog.c
gcc -fpreprocessed -dD -E -P prog.c > prog.min.c
~/git/ioccc-src/mkiocccentry/iocccsize prog.min.c
ls -l prog*.c

