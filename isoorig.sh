#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp kernel/nexa.kernel isodir/boot/nexa
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "Nexa" {
	multiboot /boot/nexa
}
EOF
grub-mkrescue -o nexa.iso isodir
