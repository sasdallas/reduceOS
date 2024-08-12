import sys

# get_modoutput.py - Returns the filename to output to.

try:
    file = open("mod.conf", "r")
except FileNotFoundError:
    print("Error: mod.conf not found!")
    sys.exit(-1)

lines = file.read().splitlines()
line = [line for line in lines if line.startswith("FILENAME ")]
line = line[0].replace("FILENAME ", "")

print(line)