/**
 * @file hexahedron/include/kernel/drivers/usb/dev.h
 * @brief USB device
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_USB_DEV_H
#define DRIVERS_USB_DEV_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/drivers/usb/desc.h>
#include <kernel/drivers/usb/req.h>

/**** DEFINITIONS ****/

// Speeds
#define USB_FULL_SPEED      0x00    // Full speed
#define USB_LOW_SPEED       0x01    // Low speed
#define USB_HIGH_SPEED      0x02    // High speed (fastest)

// Transfer statuses
#define USB_TRANSFER_IN_PROGRESS    0
#define USB_TRANSFER_FAILED         1
#define USB_TRANSFER_SUCCESS        2

// Maximum address
#define USB_MAX_ADDRESS     127     // Each controller can have at most 127 devices

/**** TYPES ****/

/**
 * @brief USB endpoint
 */
typedef struct USBEndpoint {
    USBEndpointDescriptor_t desc;   // Descriptor
    uint32_t toggle;                // For bulk data transfers
} USBEndpoint_t;


/**
 * @brief USB transfer
 * 
 * This is a basic transfer usually passed to the host controller's request (along with the device).
 */
typedef struct USBTransfer {
    uint32_t endpoint;              // Endpoint number
    USBDeviceRequest_t *req;        // Device request  
    void *data;                     // Data
    uint32_t length;                // Length of the data
    int status;                     // Transfer status (USB_TRANSFER_...)
} USBTransfer_t;

// Prototype
struct USBDevice;

// Can't include the header
struct USBController;


/**
 * @brief Host controller transfer method for a CONTROL transfer
 * @param controller The controller
 * @param dev The device
 * @param transfer The transfer
 * @returns USB_TRANSFER status code
 */
typedef int (*hc_control_t)(struct USBController *controller, struct USBDevice *dev, USBTransfer_t *transfer);


/**
 * @brief USB device
 */
typedef struct USBDevice {
    struct USBController *c;                // Controller

    // Device info
    uint32_t    port;                       // Port of the device
    int         speed;                      // Speed of the device
    uint32_t    address;                    // Address assigned to the device
    uint32_t    mps;                        // Max packet size as determined by device descriptor

    USBEndpoint_t endpoint;                 // Endpoint #0 (default control pipe)
    USBDeviceDescriptor_t device_desc;      // Device descriptor
    USBStringLanguagesDescriptor_t *langs;  // Languages (this is a pointer as we need to realloc after reading bLength)

    // HACK: Probably have to remove this
    uint16_t chosen_language;               // Chosen language for Hexahedron to use by default

    // Host controller methods
    hc_control_t    control;                // Control transfer request
} USBDevice_t;

#endif