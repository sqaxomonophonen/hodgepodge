#!/usr/bin/env bash
set -e
cd $(dirname $0)
HTML=zigdl.html
curl -s https://ziglang.org/download/ -o $HTML
latest=$(w3m -dump $HTML | grep zig-linux-$(uname -p) | head -n1 | cut -d\  -f1)
base="${latest%.tar.xz}"
if [ "$base" = $(readlink zig) ] ; then
	echo "latest is [$base]; already up to date"
	exit 0
fi
echo "latest is [$base]; updating now..."
wget https://ziglang.org/builds/$latest
echo "unpacking $latest"
tar xf $latest
if [ -L zig ] ; then
	rm zig
fi
ln -s $base zig
rm -f $HTML $latest
