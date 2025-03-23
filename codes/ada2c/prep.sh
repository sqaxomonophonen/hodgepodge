#!/usr/bin/env bash
set -e
if [ ! -e ada ] ; then
	echo "ada/ does not exist: should be a symlink to gcc/ada in the gcc source tree"
	exit 1
fi
cd ada

if [ ! -e gnat1drv.adb ] ; then
	echo "sanity check failed: expected ada/gnat1drv.adb to exist"
	exit 1
fi

gnatmake gen_il-main
./gen_il-main

gnatmake xsnamest
./xsnamest
mv snames.nh snames.h
mv snames.ns snames.ads
mv snames.nb snames.adb
