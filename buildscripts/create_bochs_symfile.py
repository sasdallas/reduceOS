#!/usr/bin/python3
import sys

#### Create a Bochs symbol file
# Bochs symbol files consist of:
# 0xADDR SYMNAME

if len(sys.argv) < 3:
    print("Usage: create_bochs_symfile.py <nm symmap> <output file>")
    sys.exit(0)

nm = open(sys.argv[1], "r")
syms = nm.readlines()
nm.close()

# Write output
out = open(sys.argv[2], "w+")
for line in syms:
    split = line.split(" ")
    out.write(split[0] + " " + split[2])

out.close()