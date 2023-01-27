#!/bin/sh
set -e
set -x
pkgs=sdl2
cc -Wall $(pkg-config --cflags ${pkgs}) shear3rotate.c -o shear3rotate $(pkg-config --libs ${pkgs}) -lm
