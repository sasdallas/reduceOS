#!/bin/bash

# iso.sh - Constructs an ISO image

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}
PROJECT_ROOT=${PROJECT_ROOT:"${BUILDSCRIPTS_ROOT}/.."}


# Change the working directory to a known path
. $BUILDSCRIPTS_ROOT/config.sh

cd $PROJECT_ROOT

# Create ISO directories
mkdir -pv $ISO_OUTPUT_DIRECTORY/iso/boot/grub/
cp $SYSROOT/$BOOT_DIRECTORY/hexahedron-kernel.elf $ISO_OUTPUT_DIRECTORY/iso/boot/
cp $SYSROOT/$BOOT_DIRECTORY/initrd.tar.img $ISO_OUTPUT_DIRECTORY/iso/boot/
cp conf/grub.cfg $ISO_OUTPUT_DIRECTORY/iso/boot/grub/
cp -r conf/extra-boot-files/* $ISO_OUTPUT_DIRECTORY/iso/boot/ || true

# Pack it into an ISO
grub-mkrescue -o $ISO_OUTPUT_DIRECTORY/hexahedron.iso $ISO_OUTPUT_DIRECTORY/iso

# Remove the old directory
rm -r $ISO_OUTPUT_DIRECTORY/iso/