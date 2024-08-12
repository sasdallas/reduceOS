# create_modconfs.py - a buildscript to create mod configurations & install them into the initial ramdisk and sysroot.

import glob, os

path = os.path.abspath("source/kmods/")
if not os.path.exists("source"):
    print("source/ does not exist, assuming we're being called from kmods Makefile")
    path = os.path.abspath("./")


boottime = {}
userspace = {}

for file in glob.glob("*/*.conf"):
    fpath = os.path.join(path, file)
    print("Assembling " + fpath + " configuration...")
    f = open(fpath, "r")
    name = ""
    priority = ""
    modload = ""

    lines = f.read().splitlines()
    f.close()
    for line in lines:
        if line.startswith("FILENAME"):
            name = line.replace("FILENAME ", "")
            print("FILENAME: " + name)
        if line.startswith("MODLOAD"):
            modload = line.replace("MODLOAD ", "")
            print("MODLOAD: " + modload)
        if line.startswith("PRIORITY"):
            priority = line.replace("PRIORITY ", "")
            print("PRIORITY: " + priority)
    
    if priority != "REQUIRED" and priority != "HIGH" and priority != "LOW":
        print("- Unknown priority: " + priority)
        continue

    if modload == "BOOTTIME":
        boottime[name] = priority
    elif modload == "USERSPACE":
        userspace[name] = priority
    else:
        print("- Unknown module load time: " + modload)
        continue

    print("- Module assembled successfully.")
    

print(f"Creating {len(boottime)} boottime modules...")
f = open(os.path.join(path, "mod_boot.conf"), "w+")
f.write("CONF_START\n")
for key, value in boottime.items():
    f.write("MOD_START\n")
    f.write("FILENAME " + key + "\n")
    f.write("PRIORITY " + value + "\n")
    f.write("MOD_END\n")
f.write("CONF_END\n")
f.close()

print(f"Creating {len(userspace)} userspace modules...")
f = open(os.path.join(path, "mod_user.conf"), "w+")
f.write("CONF_START\n")
for key, value in userspace.items():
    f.write("MOD_START\n")
    f.write("FILENAME " + key + "\n")
    f.write("PRIORITY " + value + "\n")
    f.write("MOD_END\n")
f.write("CONF_END\n")
f.close()

print("Completed successfully.")

