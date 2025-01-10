/**
 * @file hexahedron/drivers/usb/usb.c
 * @brief Main USB interface
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/usb/usb.h>
#include <kernel/drivers/clock.h>
#include <kernel/debug.h>
#include <structs/list.h>

/* Log method */
#define LOG(status, ...) dprintf_module(status, "USB", __VA_ARGS__)

/* List of USB controllers */
list_t *usb_controller_list = NULL;

/**
 * @brief Poll method (done once per tick)
 */
void usb_poll(uint64_t ticks) {
    if (!usb_controller_list) return;

    foreach(n, usb_controller_list) {
        USBController_t *controller = (USBController_t*)n->value;
        if (controller->poll) controller->poll(controller);
    }

}


/**
 * @brief Initialize the USB system (no controller drivers)
 * 
 * Controller drivers are loaded from the initial ramdisk
 */
void usb_init() {
    // Create the controller list
    usb_controller_list = list_create("usb controllers");

    // Register clock callback
    if (clock_registerUpdateCallback(usb_poll) < 0) {
        LOG(ERR, "Failed to register poll method\n");
    }

    LOG(INFO, "USB system online");
}

/**
 * @brief Register a new USB controller
 * 
 * @param controller The controller to register
 */
void usb_registerController(USBController_t *controller) {
    if (!controller) return;

    list_append(usb_controller_list, controller);
}
