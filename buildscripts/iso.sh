#!/bin/bash

# iso.sh - Constructs an ISO image

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:"${BUILDSCRIPTS_ROOT}/.."}


# Change the working directory to a known path
. $BUILDSCRIPTS_ROOT/config.sh


cd $PROJECT_ROOT

# Create these in context to the root directory
echo "-- Creating ISO directories"
mkdir -p out/iso/
mkdir -p out/iso/builddir
mkdir -p out/iso/builddir/boot
mkdir -p out/iso/builddir/boot/grub


echo "-- Copying OS files"
cp source/sysroot/boot/kernel.elf out/iso/builddir/boot/kernel.elf
cp source/sysroot/boot/ramdisk.img out/iso/builddir/boot/ramdisk.img

cat > out/iso/builddir/boot/grub/grub.cfg << EOF

insmod all_video
menuentry "reduceOS" {
    multiboot /boot/kernel.elf
    module /boot/ramdisk.img modfs=1 type=initrd
}
EOF

grub-mkrescue -o out/iso/reduceOS.iso out/iso/builddir