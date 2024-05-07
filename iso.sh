#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/nexa.kernel isodir/boot/nexa.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Nexa" {
	multiboot /boot/nexa.kernel
}
EOF
grub-mkrescue -o nexa.iso isodir
