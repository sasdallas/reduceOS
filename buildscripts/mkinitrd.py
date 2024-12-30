#!/usr/bin/python3

import tarfile 
import sys

if len(sys.argv) < 3:
    print("Usage: mkinitrd.py <tar file> <directory>")
    sys.exit(0)

file = sys.argv[1]
dir = sys.argv[2]

with tarfile.open(file, "w", format=tarfile.USTAR_FORMAT) as ramdisk:
    ramdisk.add(dir, arcname="/")