/**
 * @file hexahedron/loader/driver.c
 * @brief Hexahedron driver loader
 * 
 * Uses a JSON file located in the initial ramdisk to determine what drivers to load.
 * Keeps track of them in a hashmap which can be accessed.
 * 
 * Drivers should expose a driver_metadata field which gives a few things:
 * 1. The name of the driver
 * 2. The author of the driver (can be left as NULL) 
 * 3. The init function of the driver
 * 4. The deinit function of the driver
 * 
 * This field should be named 'driver_metadata' and be publicly exposed. It will be looked up by the ELF loader.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */