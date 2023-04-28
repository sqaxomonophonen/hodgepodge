#!/usr/bin/env bash
if [ -z "$2" ] ; then
	echo "usage: $0 <old> <new>"
	exit 1
fi
OLD="$1"
NEW="$2"
files() {
	git ls-files # filter unwanted files if you want to
}

for file in $(files) ; do
	vim -c '%s/\<'$OLD'\>/'$NEW'/gc' -c 'wq' $file
done
