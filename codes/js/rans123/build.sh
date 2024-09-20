#!/usr/bin/env bash
set -e
OPT() {
	uglifyjs $1 --compress --rename --mangle
}
OPT rans123.js > rans123.min.js
ls -l rans123.js rans123.min.js
dd if=/dev/urandom of=DATA bs=1k count=10
./base123enc.js DATA DATA.b123

out=OUT.html

cat rans123stub.html > ${out}
cat rans123.min.js >> ${out}
scalebits=14
echo -n "X=X($scalebits,\"" >> ${out}
cat DATA.b123 >> ${out}
echo    "\");" >> ${out}
echo "</script>" >> ${out}

./txtcompress.js $scalebits js_symbols.json ../player3/build3.js symtab.json symlist.txt

cc -Wall ransenc.c -o ransenc
./ransenc $scalebits symlist.txt symlist.txt.enc
ls -l ../player3/build3.js

