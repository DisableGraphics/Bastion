#!/bin/sh
set -e
. ./iso.sh

OPTIONS="-s -S"

if [ "$1" == "debug" ]; then
	qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom nexa.iso -serial file:nexa.serial -monitor stdio -s -S
else
	qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom nexa.iso -serial file:nexa.serial -monitor stdio
fi


