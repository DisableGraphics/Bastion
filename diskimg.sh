#!/bin/sh
IMAGE_FILE=disk.img
IMAGE_SIZE_MB=256
PARTITION_SIZE=$(($IMAGE_SIZE_MB - 1))
PART_NAME="BASTION"

if [ ! -f "$IMAGE_FILE" ]; then
  echo "Creating raw disk image: $IMAGE_FILE (${IMAGE_SIZE_MB}MB)"
  qemu-img create -f raw ${IMAGE_FILE} 256M
fi
LOOP_DEVICE=$(sudo losetup --find --partscan --show $IMAGE_FILE)
sync
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
sudo mkfs.vfat -F32 $PARTITION
sudo fatlabel "$PARTITION" "$PART_NAME"

echo "Mounting partition"
OUTPUT=$(sudo udisksctl mount -b "$PARTITION")
MOUNT_DIR=$(echo "$OUTPUT" | awk '{print $4}')
echo Mounted to "$MOUNT_DIR"
sudo mkdir -p "$MOUNT_DIR/dev"
sudo mkdir -p "$MOUNT_DIR/proc"
sudo mkdir -p "$MOUNT_DIR/sys"
sudo mkdir -p "$MOUNT_DIR/run"
sync
sudo mount --bind /dev "$MOUNT_DIR/dev"
sudo mount --bind /proc "$MOUNT_DIR/proc"
sudo mount --bind /sys "$MOUNT_DIR/sys"
sudo mount --bind /run "$MOUNT_DIR/run"

echo "Creating device map"
echo "(hd0) $LOOP_DEVICE" > device.map

echo "Copying OS data into partition..."
./sysroot.sh
sudo cp -r sysroot/. "$MOUNT_DIR/"

echo "Deleting sysroot"
rm -rf sysroot

echo "Installing grub so disk is bootable"
sudo grub-install --no-floppy --grub-mkdevicemap=device.map --target=i386-pc \
    --install-modules="normal multiboot" --themes="" --locales "" --boot-directory="$MOUNT_DIR" $LOOP_DEVICE

echo "Deleting device map"
rm device.map

echo "Unmounting partition"
sudo umount "$MOUNT_DIR/run"
sudo umount "$MOUNT_DIR/sys"
sudo umount "$MOUNT_DIR/proc"
sudo umount "$MOUNT_DIR/dev"
sync
sudo udisksctl unmount -b "$PARTITION"
sync
echo "Detaching loop device: $LOOP_DEVICE"
sudo losetup -d $LOOP_DEVICE
sudo kpartx -dv "$IMAGE_FILE"
sync

echo "Done! The OS is in $IMAGE_FILE."
