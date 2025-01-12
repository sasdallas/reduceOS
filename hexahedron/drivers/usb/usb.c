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
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>
#include <structs/list.h>

/* Log method */
#define LOG(status, ...) dprintf_module(status, "USB", __VA_ARGS__)

/* List of USB controllers */
list_t *usb_controller_list = NULL;

/**
 * @brief Poll method (done once per tick)
 * 
 * @note    This method needs to be replaced with probably some kernel threading system once that is implemented.
 *          Through some testing I have determined that this can and will hang the system if polls take too long
 */
void usb_poll(uint64_t ticks) {
    // !!!: polling disabled
    // if (!usb_controller_list) return;

    // foreach(n, usb_controller_list) {
    //     USBController_t *controller = (USBController_t*)n->value;
    //     if (controller->poll) controller->poll(controller);
    // }

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
 * @brief Create a USB controller
 * 
 * @param hc The host controller
 * @param poll The host controller poll method, will be called once per tick
 * @returns A @c USBController_t structure
 */
USBController_t *usb_createController(void *hc, usb_poll_t poll) {
    USBController_t *controller = kmalloc(sizeof(USBController_t));
    controller->hc = hc;
    controller->poll = poll;
    controller->devices = list_create("usb devices");
    controller->last_address = 0;

    return controller;
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
