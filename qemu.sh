#!/bin/sh
set -e
# . ./iso.sh


OPTIONS="-serial file:bastion.serial -monitor stdio -m 344M -drive id=disk,file=disk.img,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0"
DEBUG_OPTIONS="-s -S"

if [ "$1" == "debug" ]; then
	qemu-system-i386 $OPTIONS $DEBUG_OPTIONS
else
	qemu-system-i386 $OPTIONS
fi


