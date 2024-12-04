#!/bin/sh
#set -e
#. ./build.sh
#
mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp build/bastion isodir/boot/bastion
cp grub.cfg  isodir/boot/grub/grub.cfg
grub-mkrescue -o bastion.iso isodir

rm -rf isodir