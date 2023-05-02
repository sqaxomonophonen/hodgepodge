#!/usr/bin/env bash
if [ -z "$2" ] ; then
	echo "usage: $0 <old> <new>"
	exit 1
fi
OLD="$1"
NEW="$2"
PATTERN="$3"
files() {
	if [ -z "$PATTERN" ] ; then
		git ls-files
	else
		git ls-files $PATTERN
	fi
}

for file in $(files) ; do
	vim -c '%s/\<'$OLD'\>/'$NEW'/gc' -c 'wq' $file
done
