#!/bin/bash

# mkfatimage.sh - Constructs a FAT image for Polyaniline

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}


# Change the working directory to a known path
. $BUILDSCRIPTS_ROOT/config.sh
cd $BUILDSCRIPTS_ROOT/..

# Create these in context to the root directory
echo "-- Creating image output directory..."
mkdir -p out/fatimg/

echo "-- Deleting images..."
rm out/fatimg/fat_boot.img || true
rm out/fatimg/cdimage.iso || true

echo "-- Creating image..."
dd if=/dev/zero of=out/fatimg/fat_boot.img bs=1k count=1440

echo "-- Formatting image..."
mformat -i out/fatimg/fat_boot.img -f 1440 ::

echo "-- Creating directories..."
mmd -i out/fatimg/fat_boot.img ::/EFI
mmd -i out/fatimg/fat_boot.img ::/EFI/BOOT

echo "-- Copying EFI file..."
mcopy -i out/fatimg/fat_boot.img obj/polyaniline/efi/BOOTX64.EFI ::/EFI/BOOT

echo "-- Copying kernel files..."
mcopy -i out/fatimg/fat_boot.img source/sysroot/boot/kernel.elf ::/KERNEL.ELF
mcopy -i out/fatimg/fat_boot.img source/sysroot/boot/ramdisk.img ::/RAMDISK.IMG

echo "-- Temporary: Creating CDROM image..."
rm -r out/fatimg/isodir || true
mkdir -p out/fatimg/isodir
cp out/fatimg/fat_boot.img out/fatimg/isodir/
cp source/sysroot/boot/kernel.elf out/fatimg/isodir/kernel
cp source/sysroot/boot/ramdisk.img out/fatimg/isodir/initrd
xorriso -as mkisofs -R -f -e fat_boot.img -no-emul-boot -o out/fatimg/cdimage.iso out/fatimg/isodir



echo "-- Created image successfully!"
