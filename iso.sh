#!/bin/sh
#set -e
#. ./build.sh
#
mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp build/bastion isodir/boot/bastion
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Bastion" {
	multiboot /boot/bastion
}
EOF
grub-mkrescue -o bastion.iso isodir
