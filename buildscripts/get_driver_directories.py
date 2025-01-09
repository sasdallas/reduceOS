#!/usr/bin/python3
import sys
import os

# get_driver_directories.py - Parses and finds driver directories according to build architecture

if len(sys.argv) < 2:
    print("Usage: get_driver_directories.py <build arch>")
    sys.exit(-1)

arch = sys.argv[1].lower()

# Walk through and get all subdirectories
dirs = next(os.walk("."))[1]

compatible_directories = "" # This will be printed out at the end

# For each subdirectory, check the driver.conf file in it
for dir in dirs:
    lines = []
    try:    
        conf = open(os.path.join(dir, "driver.conf"), "r")
        lines = [line.strip() for line in conf.readlines()]
        conf.close()
    except FileNotFoundError:
        continue

    # Now find the line with ARCH in it
    arch_line = [line for line in lines if line.startswith("ARCH = ")][0].replace("ARCH = ", "")
    
    # ARCH = ANY?
    if arch_line == "ANY":
        compatible_directories = compatible_directories + " " + dir
        continue

    # No, but they can specify multiple architectures
    potential_architectures = [a.lower() for a in arch_line.split(" OR ")]
    if arch in potential_architectures:
        compatible_directories = compatible_directories + " " + dir

print(compatible_directories)