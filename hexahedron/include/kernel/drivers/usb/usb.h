/**
 * @file hexahedron/include/kernel/drivers/usb/usb.h
 * @brief Universal Serial Bus driver
 * 
 * @note This USB driver is mostly sourced from the previous reduceOS. It will likely need to be expanded and added upon. 
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
#include <structs/list.h>

// USB
#include <kernel/drivers/usb/dev.h>
#include <kernel/drivers/usb/driver.h>
#include <kernel/drivers/usb/status.h>

/**** TYPES ****/

// Prototype
struct USBController;

/**
 * @brief Poll method for USB controller
 * 
 * @warning Currently unused as of Jan 16, 2025
 * 
 * @param controller The controller
 */
typedef void (*usb_poll_t)(struct USBController *controller);

/**
 * @brief USB controller structure
 * 
 * Normal USB drivers do not need to register this. This is for host controller
 * drivers, such as xHCI/EHCI
 */
typedef struct USBController {
    void *hc;               // Pointer to the host controller structure
    usb_poll_t poll;        // Poll method, will be called once every tick
    list_t *devices;        // List of USB devices with a maximum of 127
    uint32_t last_address;  // Last address given to a device. Starts at 0x1
} USBController_t;


/**** FUNCTIONS ****/

/**
 * @brief Initialize the USB system (no controller drivers)
 * 
 * Controller drivers are loaded from the initial ramdisk
 */
void usb_init();

/**
 * @brief Create a USB controller
 * 
 * @param hc The host controller
 * @param poll The host controller poll method, will be called once per tick
 * @returns A @c USBController_t structure
 */
USBController_t *usb_createController(void *hc, usb_poll_t poll);

/**
 * @brief Register a new USB controller
 * 
 * @param controller The controller to register
 */
void usb_registerController(USBController_t *controller);

/**
 * @brief Initialize a USB device and assign to the USB controller's list of devices
 * @param dev The device to initialize
 * 
 * @returns Negative value on failure and 0 on success
 */
USB_STATUS usb_initializeDevice(USBDevice_t *dev);

/**
 * @brief Create a new USB device structure for initialization
 * @param controller The controller
 * @param port The device port
 * @param speed The device speed
 * 
 * @param control The HC control request method
 */
USBDevice_t *usb_createDevice(USBController_t *controller, uint32_t port, int speed, hc_control_t control);

/**
 * @brief Destroy a USB device
 * @param controller The controller
 * @param dev The device to destroy
 * 
 * @warning Does not shut the device down, just frees it from memory
 */
void usb_destroyDevice(USBController_t *controller, USBDevice_t *dev);

/**
 * @brief USB device request method
 * 
 * @param device The device
 * @param type The request type (should have a direction, type, and recipient - bmRequestType)
 * @param request The request to send (USB_REQ_... - bRequest)
 * @param value Optional parameter to the request (wValue)
 * @param index Optional index for the request (this field differs depending on the recipient - wIndex)
 * @param length The length of the data (wLength)
 * @param data The data
 * 
 * @returns The transfer status (not USB_STATUS)
 */
int usb_requestDevice(USBDevice_t *device, uintptr_t type, uintptr_t request, uintptr_t value, uintptr_t index, uintptr_t length, void *data);

/**
 * @brief Read a string from the USB device
 * @param dev The device to read the string from
 * @param idx The index of the string to read
 * @param lang The language code
 * @returns ASCII string (converted from the normal unicode)
 */
char *usb_getStringIndex(USBDevice_t *device, int idx, uint16_t lang);

#endif