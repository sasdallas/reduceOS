#!/usr/bin/python3

# get_userspace_app_info.py - Returns a variety of information about a userspace app

import sys
import os

if len(sys.argv) < 3:
    print("Usage: get_userspace_app_info.py <working dir> <request>")
    print("Available requests: NAME, INSTALL_DIR, CFLAGS, LIBS, ADDITIONAL, INITRD")
    sys.exit(1)

WORKING_DIR = sys.argv[1]
REQUEST = sys.argv[2]


APP_FILE = os.path.join(WORKING_DIR, "app.conf")
if not os.path.exists(APP_FILE):
    raise FileNotFoundError("app.conf not found in " + WORKING_DIR)

f = open(APP_FILE, "r")
lines = f.read().splitlines()
f.close()

# Check request
if REQUEST == "NAME":
    # Name of the app
    for line in lines:
        if line.startswith("APP_NAME = "):
            print(line.replace("\"", "").replace("APP_NAME = ", ""))
            sys.exit(0)

    raise ValueError("APP_NAME not found in app.conf")
elif REQUEST == "INSTALL_DIR":
    # Where the app should be installed
    for line in lines:
        if line.startswith("INSTALL_DIR = "):
            print(line.replace("\"", "").replace("INSTALL_DIR = ", ""))
            sys.exit(0)

    raise ValueError("INSTALL_DIR not found in app.conf")
elif REQUEST == "CFLAGS":
    # CFLAGS to use while compiling the app
    for line in lines:
        if line.startswith("CFLAGS = "):
            print(line.replace("CFLAGS = ", ""))
            sys.exit(0)

    sys.exit(0)
elif REQUEST == "LIBS":
    # LIBS to use while compiling the app.
    # Basically LDFLAGS, so the user can specify additional lib directories if needed
    for line in lines:
        if line.startswith("LIBS = "):
            print(line.replace("LIBS = ", ""))
            sys.exit(0)

    sys.exit(0)
elif REQUEST == "ADDITIONAL":
    # Additional Makefile targets to run
    for line in lines:
        if line.startswith("ADDITIONAL = "):
            print(line.replace("ADDITIONAL = ", ""))
            sys.exit(0)
    sys.exit(0)
elif REQUEST == "INITRD":
    # Install into temporary /bin directory on initrd
    for line in lines:
        if line.startswith("INITRD"):
            print("YES")
            sys.exit(0)
    sys.exit(0)
else:
    raise ValueError("Invalid REQUEST: " + REQUEST)


