#!/bin/sh
set -e
. ./isoorig.sh


OPTIONS="-cdrom nexa.iso -serial file:nexa.serial -monitor stdio -m 344M"
DEBUG_OPTIONS="-s -S"

if [ "$1" == "debug" ]; then
	qemu-system-i386 $OPTIONS $DEBUG_OPTIONS
else
	qemu-system-i386 $OPTIONS
fi


