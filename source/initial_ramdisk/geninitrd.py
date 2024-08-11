#!/usr/bin/python3 

import os 
import tarfile
import sys

# geninitrd.py - Generates an initial ramdisk for reduceOS using directories are arguments

print("geninitrd v1.0.0 by @sasdallas")

if len(sys.argv) != 3:
    print("Usage: geninitrd.py <output.tar> <root directory>")
    sys.exit(-1)

output = sys.argv[1]
rootdir = sys.argv[2]

print(f"output tar archive: {output}")
print(f"root directory: {rootdir}")


with tarfile.open(output, "w", format=tarfile.USTAR_FORMAT) as ramdisk:
    ramdisk.add(rootdir, arcname="/")

print("initrd created succesfully.")