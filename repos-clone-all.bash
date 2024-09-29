#!/usr/bin/env bash
set -e
lst=$(dirname $0)/repos.list
if [ ! -e "$lst" ] ; then
	echo "cannot find repos.list"
	exit 1
fi
for repo in $(cat $lst) ; do
	if [ -d "$repo" ] ; then
		echo HAVE $repo
		continue
	fi
	echo cloning $repo
	git clone git@github.com:sqaxomonophonen/$repo.git
done
