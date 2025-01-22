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


# For each subdirectory, check the driver.conf file in it
def walk_dir(path, dirs):
    found_directories = ""
    for dir in dirs:
        lines = []

        # First get the full directory path
        dirpath = os.path.join(path, dir)

        # Try to open driver.conf
        try:    
            conf = open(os.path.join(dirpath, "driver.conf"), "r")
            lines = [line.strip() for line in conf.readlines()]
            conf.close()
        except FileNotFoundError:
            # Does the directory contain "make.config"? If so it's a driver subdirectory
            if os.path.exists(os.path.join(path, "make.config")):
                # Driver subdirectory
                found_directories = found_directories + walk_dir(dirpath, next(os.walk(dirpath))[1])
            continue

        # Now find the line with ARCH in it
        arch_line = [line for line in lines if line.startswith("ARCH = ")][0].replace("ARCH = ", "")
        
        # Fix dirpath, Makefile will replace "-" with "/"
        dirpath = dirpath.replace("./", "").replace("/", "-")

        # ARCH = ANY?
        if arch_line == "ANY":
            found_directories = found_directories + " " + dirpath
            continue

        # No, but they can specify multiple architectures
        potential_architectures = [a.lower() for a in arch_line.split(" OR ")]
        if arch in potential_architectures:
            found_directories = found_directories + " " + dirpath

    return found_directories

driver_dirs = walk_dir(".", dirs)
print(driver_dirs)