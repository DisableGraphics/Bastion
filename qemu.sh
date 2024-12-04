#!/bin/sh
set -e
. ./iso.sh


OPTIONS="-cdrom bastion.iso -serial file:bastion.serial -monitor stdio -m 344M -hda disk.img -boot menu=on"
DEBUG_OPTIONS="-s -S"

if [ "$1" == "debug" ]; then
	qemu-system-i386 $OPTIONS $DEBUG_OPTIONS
else
	qemu-system-i386 $OPTIONS
fi


