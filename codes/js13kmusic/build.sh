#!/bin/sh
cp index.template.html index.html
sed -i "" "s/@@@SONG@@@/$(./dump.sh)/" index.html
