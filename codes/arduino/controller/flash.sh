#!/bin/sh
arduino-avrdude -C/usr/local/etc/arduino-avrdude.conf -v -V -b115200 -patmega2560 -cwiring -D -Uflash:w:controller.hex:i -P/dev/cuaU0
