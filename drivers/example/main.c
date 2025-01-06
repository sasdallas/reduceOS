/**
 * @file drivers/example/main.c
 * @brief Example driver for Hexahedron
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/loader/driver.h>
#include <kernel/debug.h>
#include <stdio.h>


int driver_init(int argc, char **argv) {
    printf("This is 'example_driver' speaking. Hello Hexahedron!\n");
    dprintf(DEBUG, "Debugging!\n");
    return 0;
}

int driver_deinit() {
    return 0;
}

struct driver_metadata driver_metadata = {
    .name = "Example Driver",
    .author = "Samuel Stuart",
    .init = driver_init,
    .deinit = driver_deinit
};

