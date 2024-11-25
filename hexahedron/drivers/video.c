/**
 * @file hexahedron/drivers/video.c
 * @brief Generic video driver
 * 
 * This video driver system handles abstracting the video layer away.
 * It supports text-only video drivers (but may cause gfx display issues) and it
 * supports pixel-based video drivers.
 * 
 * Currently, implementation is rough because we are at the beginnings of a new kernel,
 * but here is what is planned:
 * - Implement a list system to allow choosing
 * - Allow arguments to modify this
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/video.h>
#include <structs/list.h>
#include <string.h>


static list_t *video_driver_list = NULL;
static video_driver_t *current_driver = NULL;

/**
 * @brief Initialize and prepare the video system.
 * 
 * This doesn't actually initialize any drivers, just starts the system.
 */
void video_init() {
    video_driver_list = list_create("video drivers");
}

/**
 * @brief Add a new driver
 * @param driver The driver to add
 */
void video_addDriver(video_driver_t *driver) {
    if (!driver) return;

    list_append(video_driver_list, driver);
}

/**
 * @brief Switch to a specific driver
 * @param driver The driver to switch to. If not found in the list it will be added.
 */
void video_switchDriver(video_driver_t *driver) {
    if (!driver) return;

    if (list_find(video_driver_list, driver) == NULL) {
        video_addDriver(driver);
    }

    current_driver = driver; // TODO: If this interface gets more flushed out, drivers will need a load/unload method.
}

/**
 * @brief Find a driver by name
 * @param name The name to look for
 * @returns NULL on not found, else it returns a driver
 */
video_driver_t *video_findDriver(char *name) {
    foreach(driver_node, video_driver_list) {
        if (!strcmp(((video_driver_t*)driver_node->value)->name, name)) {
            return (video_driver_t*)driver_node->value;
        }
    }

    return NULL;
}

/**** INTERFACING FUNCTIONS ****/

/**
 * @brief Plot a pixel on the screen
 * @param x The x coordinate of where to plot the pixel
 * @param y The y coordinate of where to plot the pixel
 * @param color The color to plot
 */
void video_plotPixel(int x, int y, color_t color) {
    if (current_driver != NULL && current_driver->putpixel) {
        current_driver->putpixel(current_driver, x, y, color);
    }
}

/**
 * @brief Clear the screen with colors
 * @param bg The background of the screen
 */
void video_clearScreen(color_t bg) {
    if (current_driver != NULL && current_driver->clear) {
        current_driver->clear(current_driver, bg);
    }
}

/**
 * @brief Update the screen
 */
void video_updateScreen() {
    if (current_driver != NULL && current_driver->update) {
        current_driver->update(current_driver);
    }
}