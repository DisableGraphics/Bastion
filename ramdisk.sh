#!/bin/sh

RAMDISK_FOLDER="ramdisk"
RAMDISK="ramdisk.tar"

echo "Creating ramdisk: $RAMDISK"
mkdir -p "$RAMDISK_FOLDER"
echo "1" > "$RAMDISK_FOLDER/boot_partition"
cp fonts/console.sfn "$RAMDISK_FOLDER/console.sfn"
tar -cf "$RAMDISK" "$RAMDISK_FOLDER"
rm -rf "$RAMDISK_FOLDER"
