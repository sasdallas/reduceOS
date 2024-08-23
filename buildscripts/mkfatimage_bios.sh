#!/bin/bash

# mkfatimage_bios.sh - Creates a FAT12 image for BIOS configurations of reduceOS
set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}


# Change the working directory to a known path
. $BUILDSCRIPTS_ROOT/config.sh
cd $BUILDSCRIPTS_ROOT/..


echo "-- Removing old image..."
mkdir -pv out/fatimg || true
rm -rf out/fatimg/fatbios.img || true

echo "-- Creating fat.img..."
mkfs.msdos -C out/fatimg/fatbios.img 1440

echo "-- Formatting fat.img..."
mformat -i out/fatimg/fatbios.img -f 1440 -B obj/polyaniline/bios/mbr.bin ::

echo "-- Copying files..."
mcopy -i out/fatimg/fatbios.img obj/polyaniline/bios/boot.sys ::/BOOT.SYS

echo "-- Done!"


