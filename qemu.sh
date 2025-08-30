#!/bin/sh
COMMMON_OPTS="-boot d -cdrom image.iso -m 5120 -serial file:bastion.serial -smp 3 -no-reboot -d int,mmu -D qemu.log"
if [ "$1" == "debug" ]; then
	qemu-system-x86_64 $COMMMON_OPTS -s -S -daemonize
else
	qemu-system-x86_64 $COMMMON_OPTS 
fi