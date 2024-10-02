#!/bin/bash

# iso9660 version of mkfatimage

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}


# Change the working directory to a known path
. $BUILDSCRIPTS_ROOT/config.sh
cd $BUILDSCRIPTS_ROOT/..


# Let's start. Delete any remnants
rm -r out/fatimg/iso9660_dir | true
mkdir -pv out/fatimg/iso9660_dir/boot

# Copy the files
cp source/sysroot/boot/kernel.elf out/fatimg/iso9660_dir/boot/
cp source/sysroot/boot/ramdisk.img out/fatimg/iso9660_dir/boot/
cp obj/polyaniline/bios/mbr.bin out/fatimg/iso9660_dir/boot/
cp obj/polyaniline/bios/boot.sys out/fatimg/iso9660_dir/boot/stage2.bin

mkisofs -R -b boot/mbr.bin -no-emul-boot -V REDUCE -v -o out/fatimg/bios_cdimage.iso out/fatimg/iso9660_dir/

echo Done!
