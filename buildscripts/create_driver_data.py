#!/usr/bin/python3

import sys
import json

# create_driver_data.py - Create a JSON object with driver data
# Expects a bunch of directories to be passed containing driver.confs

# !!!!!!!!!!!!!!!! EDIT THESE VARIABLES IF YOU CHANGED driver.h
DRIVER_ENVIRONMENT_NORMAL = 0
DRIVER_ENVIRONMENT_PRELOAD = 1
DRIVER_ENVIRONMENT_ANY = 2

DRIVER_VERSION = 1

DRIVER_CRITICAL = 0
DRIVER_WARN = 1
DRIVER_IGNORE = 2


data = {"version": DRIVER_VERSION, "drivers": []}

for i in range(1, len(sys.argv)):
    driver = sys.argv[i]
    
    # Try to open the config file
    conflines = []

    try:
        conf = open(f"{driver}/driver.conf", "r")
        conflines = [line.strip() for line in conf.readlines()]
        conf.close()
    except Exception:
        raise FileNotFoundError(f"{driver}/driver.conf not found")
    
    filename = [line for line in conflines if line.startswith("FILENAME = ")][0].replace("\"", "").replace("FILENAME = ", "")
    priority = [line for line in conflines if line.startswith("PRIORITY = ")][0].replace("PRIORITY = ", "")
    environment = [line for line in conflines if line.startswith("ENVIRONMENT = ")][0].replace("ENVIRONMENT = ", "")

    driver_data = {"filename": filename, "priority": DRIVER_CRITICAL, "environment": DRIVER_ENVIRONMENT_ANY}

    # Translate priority to number
    if priority == "CRITICAL":
        driver_data["priority"] = DRIVER_CRITICAL
    elif priority == "WARN":
        driver_data["priority"] = DRIVER_WARN
    elif priority == "IGNORE":
        driver_data["priority"] = DRIVER_IGNORE
    else:
        print(f"WARNING: Driver \"{filename}\" has unrecognized priority \"{priority}\". Assuming IGNORE")
        driver_data["priority"] = DRIVER_IGNORE

    # Translate environment to number
    if environment == "NORMAL":
        driver_data["environment"] = DRIVER_ENVIRONMENT_NORMAL
    elif environment == "PRELOAD":
        driver_data["environment"] = DRIVER_ENVIRONMENT_PRELOAD
    elif environment == "ANY":
        driver_data["environment"] = DRIVER_ENVIRONMENT_ANY
    else:
        print(f"WARNING: Driver \"{filename}\" has unrecognized environment \"{environment}\". Assuming NORMAL")
        driver_data["environment"] = DRIVER_ENVIRONMENT_NORMAL

    print(f"-- {filename} will be loaded with priority {priority} and environment {environment}")

    data["drivers"].append(driver_data)

# Write the data to the file
output = open("driver_conf.json", "w+")
output.write(json.dumps(data))
output.close()

# All done!