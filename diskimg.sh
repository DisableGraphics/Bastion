#!/bin/sh
IMAGE_FILE=disk.img
IMAGE_SIZE_MB=256
PARTITION_SIZE=$(($IMAGE_SIZE_MB - 1))
MOUNT_DIR=mnt

if [ ! -f "$IMAGE_FILE" ]; then
  echo "Creating raw disk image: $IMAGE_FILE (${IMAGE_SIZE_MB}MB)"
  qemu-img create -f raw ${IMAGE_FILE} 256M
fi
LOOP_DEVICE=$(sudo losetup --find --partscan --show $IMAGE_FILE)
if [ -z "$LOOP_DEVICE" ]; then
  echo "Failed to attach loop device"
  exit 1
fi
echo "Attached loop device: $LOOP_DEVICE"

echo "Creating MBR partition table on $LOOP_DEVICE"
sudo parted --script $LOOP_DEVICE mklabel msdos
sudo parted --script $LOOP_DEVICE mkpart primary fat32 1MiB ${PARTITION_SIZE}MiB


PARTITION="${LOOP_DEVICE}p1"
if [ ! -e "$PARTITION" ]; then
  echo "Partition $PARTITION not found. Trying kpartx."
  kpartx -a $LOOP_DEVICE  # Add partition mappings using kpartx
  PARTITION="/dev/mapper/$(basename ${LOOP_DEVICE})p1"
fi
echo "Formatting the partition as FAT32: $PARTITION"
sudo mkfs.fat -F32 $PARTITION

echo "Mounting partition"
mkdir -p "$MOUNT_DIR"
sudo mount $PARTITION "$MOUNT_DIR"

echo "Creating device map"
echo "(hd0) $LOOPBACK_DEVICE" > device.map

echo "Copying OS data into partition..."
./sysroot.sh
sudo cp -r sysroot/. "$MOUNT_DIR/"

echo "Deleting sysroot"
rm -rf sysroot

echo "Installing grub so disk is bootable"
sudo grub-install --no-floppy --grub-mkdevicemap=device.map --target=i386-pc \
    --modules="part_msdos" --boot-directory="$MOUNT_DIR" /dev/loop0 -v

echo "Deleting device map"
rm device.map

echo "Unmounting partition"
sudo umount "$MOUNT_DIR"
rm -rf "$MOUNT_DIR"
echo "Detaching loop device: $LOOP_DEVICE"
sudo losetup -d $LOOP_DEVICE
sudo kpartx -d $LOOP_DEVICE 2>/dev/null

echo "Done! The OS is in $IMAGE_FILE."