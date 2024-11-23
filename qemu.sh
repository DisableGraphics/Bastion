#!/bin/sh
set -e
. ./iso.sh


OPTIONS="-cdrom nexa.iso -serial file:nexa.serial -monitor stdio -m 344M"
DEBUG_OPTIONS="-s -S"

if [ "$1" == "debug" ]; then
	qemu-system-$(./target-triplet-to-arch.sh $HOST) $OPTIONS $DEBUG_OPTIONS
else
	qemu-system-$(./target-triplet-to-arch.sh $HOST) $OPTIONS
fi


