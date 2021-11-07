#!/usr/bin/env bash

value=${1:-1}

id=$(xinput list | perl -ne '/Synaptics.*id=(\d+)/ && print "$1";')
if [ -z "$id" ] ; then
	echo "Synaptics device not found"
	exit 1
fi

for prop in $(xinput list-props ${id} | perl -ne '/Disable While Typing Enabled \((\d+)\)/ && print " $1";') ; do
	echo xinput set-prop ${id} ${prop} ${value}
	xinput set-prop ${id} ${prop} ${value}
done
