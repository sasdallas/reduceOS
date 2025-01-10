/**
 * @file hexahedron/include/kernel/drivers/usb/usb.h
 * @brief Universal Serial Bus driver
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_USB_USB_H
#define DRIVERS_USB_USB_H

/**** INCLUDES ****/
#include <stdint.h>

/**** TYPES ****/

// Prototype
struct USBController;

/**
 * @brief Poll method for USB controller
 * 
 * @param controller The controller
 */
typedef void (*usb_poll_t)(struct USBController *controller);

/**
 * @brief USB controller structure
 */
typedef struct USBController {
    void *hc;           // Pointer to the host controller structure
    usb_poll_t poll;    // Poll method, will be called once every tick
} USBController_t;


/**** FUNCTIONS ****/

/**
 * @brief Initialize the USB system (no controller drivers)
 * 
 * Controller drivers are loaded from the initial ramdisk
 */
void usb_init();

#endif