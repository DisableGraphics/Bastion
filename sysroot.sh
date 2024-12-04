#!/bin/sh
echo "Creating directory structure for system root"
mkdir -p sysroot
mkdir -p sysroot/boot
mkdir -p sysroot/grub
cp build/bastion sysroot/boot
cp grub.cfg sysroot/grub