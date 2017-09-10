#!/bin/sh
unzip -p xem.xrns Song.xml | $(dirname $0)/xrns2js.py $1 $2
