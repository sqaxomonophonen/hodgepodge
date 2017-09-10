#!/bin/sh
unzip -p $1 Song.xml | $(dirname $0)/xrns2js.py
