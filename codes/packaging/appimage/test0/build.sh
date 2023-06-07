#!/usr/bin/env bash
set -e
rm -rf AppDir
make test0
mkdir AppDir
cp test0 AppDir/AppRun
cp test0.desktop AppDir/
cp test0.png AppDir/
# try this to reassure yourself that there's no startup overhead for large
# .AppImage files:
#dd bs=1M count=500 if=/dev/urandom of=AppDir/IMPORTANT_GARBAGE
appimagetool-x86_64.AppImage AppDir/
