#!/usr/bin/env bash
set -e
OPT() {
	uglifyjs $1 --compress --rename --mangle
}
OPT rans123.js > rans123.min.js
ls -l rans123.js rans123.min.js

scalebits=14

###   dd if=/dev/urandom of=DATA bs=1k count=10
###   ./base123enc.js DATA DATA.b123
###   out=OUT.html
###   cat rans123stub.html > ${out}
###   cat rans123.min.js >> ${out}
###   echo -n "X=X($scalebits,\"" >> ${out}
###   cat DATA.b123 >> ${out}
###   echo    "\");" >> ${out}
###   echo "</script>" >> ${out}


list=symlist.txt
tab=symtab.json
./txtcompress.js $scalebits js_symbols.json ../player3/build3.js ${tab} ${list}

cc -Wall ransenc.c -o ransenc
enc="${list}.enc"
./ransenc $scalebits ${list} ${enc}
ls -l ../player3/build3.js
encjstr="${enc}.jstr"
./base123enc.js ${enc} ${encjstr}

out="PTEST.html"
cat rans123stub.html > ${out}
cat rans123.min.js >> ${out}
#cat rans123.js >> ${out}
echo -n "X=X($scalebits,\"" >> ${out}
cat ${encjstr} >> ${out}
echo    "\");" >> ${out}
echo -n "S=" >> ${out}
cat ${tab} >> ${out}
echo >> ${out}
cat decode_test_suffix.js >> ${out}
echo "</script>" >> ${out}
ls -l ${out}
