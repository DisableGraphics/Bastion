#!/bin/sh
set -e
. ./iso.sh

OPTIONS="-s -S"

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom nexa.iso -serial file:nexa.serial -monitor stdio -s -S
