import os
import sys
import datetime
if len(sys.argv) < 2:
    print("Usage: update_build_no_and_date.py <release/debug>")
    sys.exit(1)

if sys.argv[1].lower() != "release" and sys.argv[1].lower() != "debug":
    print("Usage: update_build_no_and_date.py <release/debug>")
    sys.exit(1)

print("You are currently building reduceOS as a " + str(sys.argv[1]) + " build.")

with open("source/kernel/include/CONFIG.h", "r") as f:
    lines = f.readlines()
    f.close()

build_no = lines[12][22:]
build_no = int(build_no.replace("\"", "")) + 1

build_date = datetime.datetime.now().strftime("%m/%d/%y, %H:%M:%S")

print("Build number: " + str(build_no))
print("Build date: " + str(build_date))

lines[12] = "#define BUILD_NUMBER \"" + str(build_no) + "\"\n"
lines[13] = "#define BUILD_DATE \"" + build_date + "\"\n"
lines[14] = "#define BUILD_CONFIGURATION \"" + sys.argv[1].upper() + "\"\n"

with open("source/kernel/include/CONFIG.h", "w") as f:
    f.write("".join(lines))
    f.close()