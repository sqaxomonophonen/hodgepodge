#!/usr/bin/env bash
set -e
cd $(dirname $0)
build_dir="build"
mkdir -p $build_dir

if [ "$PACKART" = "1" ] ; then
	cd art
	./packart.sh
	cd ..
fi

splice() {
	source_token="$1"
	src_path="$2"
	insert_path="$3"
	dst_path="$4"

	split_point=$(grep -n $source_token $src_path | cut -d: -f1)
	cat \
		<(head -n$(( $split_point - 1 )) $src_path) \
		$insert_path \
		<(tail -n+$(( $split_point + 1 )) $src_path) \
		> $dst_path
}

sz() {
	label="$1:"
	while [ "$(( ${#label} < 30 ))" = "1" ] ; do
		label="$label "
	done
	echo -n "$label"
	size=$(stat --format "%s" $1)
	if [ -n "$2" ] ; then
		echo "$size/$2 bytes ($(( $size * 100 / $2 ))%)"
	else
		echo "$size bytes"
	fi
}

splice INSERT_CODEGEN_HERE game.js "art/codegen-packed-art.js" $build_dir/game.nomin.js

if [ "$FAST" = "1" ] ; then
	final_js=$build_dir/game.nomin.js
else
	npx uglify-js $build_dir/game.nomin.js --compress --mangle toplevel --mangle-props regex=/^_/ -o $build_dir/game.min.js
	sz game.js
	sz $build_dir/game.min.js

	final_js=$build_dir/game.min.js # TODO try other stuff?
fi

artifact=$build_dir/game.index.html
splice GAME_SOURCE game.html $final_js $artifact
sz $artifact
rm -f $build_dir/game.zip
zip -9 $build_dir/game.zip $artifact # try without -9?
sz $build_dir/game.zip
advzip -z -4 $build_dir/game.zip
sz $build_dir/game.zip $((13 * 1024))

echo $artifact
