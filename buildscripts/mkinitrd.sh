#!/bin/sh

# mkinitrd.sh - creates a ustar archive of the initial ramdisk

set -e

BUILDSCRIPTS_ROOT=${BUILDSCRIPTS_ROOT:-"$(cd `dirname $0` && pwd)"}

tar --format=ustar -cvf $BUILDSCRIPTS_ROOT/../build-output/sysroot/boot/initrd.tar.img $BUILDSCRIPTS_ROOT/../build-output/initrd/ 