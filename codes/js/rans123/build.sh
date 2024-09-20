#!/usr/bin/env bash
set -e
OPT() {
	uglifyjs $1 --compress --rename --mangle
}
OPT rans123.js > rans123.min.js
ls -l rans123.js rans123.min.js
cc -Wall ransenc.c -o ransenc
dd if=/dev/urandom of=DATA bs=1k count=10
./base123enc.js DATA DATA.b123

cat rans123stub.html > OUT.html
echo -n "X=X(14,\"" >> OUT.html
cat DATA.b123 >> OUT.html
echo    "\");" >> OUT.html
echo "</script>" >> OUT.html
