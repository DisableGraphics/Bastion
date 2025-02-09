#!/bin/sh
echo "Creating directory structure for system root"
mkdir -p sysroot
mkdir -p sysroot/boot
mkdir -p sysroot/bin
mkdir -p sysroot/cfg
mkdir -p sysroot/data
mkdir -p sysroot/grub
cp build/bastion sysroot/boot

executables=$(find build -iname "*.exe")

for i in $executables
do
	cp "$i" sysroot/bin
done

echo "hello world from a single file" > sysroot/data/test.txt
echo "true" > sysroot/boot/is_bastion
cp grub.cfg sysroot/grub