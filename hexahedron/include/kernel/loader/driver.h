/**
 * @file hexahedron/include/kernel/loader/driver.h
 * @brief Driver loader and metadata structure
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_LOADER_DRIVER_H
#define KERNEL_LOADER_DRIVER_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/fs/vfs.h>

/**** TYPES ****/

/* These types are exposed to drivers, so be fancy! */

/**
 * @brief The driver initialization function.
 * @param argc The number of arguments passed to the driver.
 * @param argv A pointer to a list containing the arguments
 * @returns 0 on success, anything else is failure.
 */
typedef int (*driver_init_t)(int argc, char **argv);

/**
 * @brief The driver deinitialization function.
 * @returns 0 on success, anything else is failure.
 */
typedef int (*driver_deinit_t)();

/**
 * @brief The main driver metadata structure. All drivers need this.
 * 
 * Please expose this as @c driver_metadata in your driver.
 */
typedef struct driver_metadata {
    char *name;                 // The name of the driver (REQUIRED)
    char *author;               // The author of the driver (OPTIONAL, leave as NULL if you want)
    driver_init_t init;         // Init function of the driver
    driver_deinit_t deinit;     // Deinit function of the driver
} driver_metadata_t;

/**** DEFINITIONS ****/

// The default location of the driver file
#define DRIVER_DEFAULT_CONFIG_LOCATION      "/device/initrd/drivers/driver_conf.json"

// Make sure to update buildscripts/create_driver_data.py if you change this
#define DRIVER_CRITICAL     0   // Panic if load fails
#define DRIVER_WARN         1   // Warn the user if load fails
#define DRIVER_IGNORE       2   // Ignore if load fails

/**** FUNCTIONS ****/

/**
 * @brief Initialize the driver loading system (this doesn't load anything)
 */
void driver_initialize();

/**
 * @brief Load and parse a JSON file containing driver information
 * @param file The file to parse
 * @returns 0 on success, anything else indicates a failure.
 * 
 * @note This will panic if any drivers have the label of "CRITICAL"
 */
int driver_loadConfiguration(fs_node_t *file);

/**
 * @brief Load a driver into memory and start it
 * @param driver_file The driver file
 * @returns 0 on success, anything else is a failure
 */
int driver_load(fs_node_t *driver_file);


#endif