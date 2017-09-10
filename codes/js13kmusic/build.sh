#!/bin/sh
cp index.template.html index.html
sed -i "" "s/@@@SONG1@@@/$(./dump.sh xem.xrns)/" index.html
sed -i "" "s/@@@SONG2@@@/$(./dump.sh xem2.xrns)/" index.html
