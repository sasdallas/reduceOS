/**
 * @file hexahedron/include/kernel/drivers/usb/desc.h
 * @brief USB descriptors
 * 
 * @see https://wiki.osdev.org/Universal_Serial_Bus#Standard_USB_Descriptors
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_USB_DESC_H
#define DRIVERS_USB_DESC_H

/**** INCLUDES ****/

#include <stdint.h>

/**** DEFINITIONS ****/

// Descriptor types
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONF           0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTF           0x04
#define USB_DESC_ENDP           0x05

// HID types
#define USB_DESC_HID            0x21
#define USB_DESC_REPORT         0x22
#define USB_DESC_PHYSICAL       0x23

// Hub types
#define USB_DESC_HUB            0x29

// Endpoint usage types
#define USB_ENDP_USAGE          0x30    // Bitmask
#define USB_ENDP_DATA           0x00
#define USB_ENDP_FEEDBACK       0x10
#define USB_ENDP_FEEDBACK_IMPL  0x30

// TODO: Endpoint transfer types and syncronization types

/**** TYPES ****/

/**
 * @brief USB device descriptor
 * 
 * Describes general information about a USB device.
 * 
 * @note    If a device is high-speed (and has different device information for each speed),
 *          it must have a @c USBDeviceQualifierDescriptor descriptor
 */
typedef struct USBDeviceDescriptor {
    uint8_t     bLength;            // Size of this descriptor in bytes
    uint8_t     bDescriptorType;    // USB_DESC_DEVICE
    uint16_t    bcdUSB;             // USB specification release number in BCD

    uint8_t     bDeviceClass;       // Class code
    uint8_t     bDeviceSubClass;    // Subclass code
    uint8_t     bMaxPacketSize0;    // Maximum packet size for the endpoint 0 (8/16/32/64 only valid)
    
    uint16_t    idVendor;           // Vendor ID
    uint16_t    idProduct;          // Product ID
    uint16_t    bcdDevice;          // Device release number in BCD
    
    uint8_t     iManufacturer;      // Index of STRING descriptor describing manufacturer
    uint8_t     iProduct;           // Index of STRING descriptor describing product
    uint8_t     iSerialNumber;      // Index of STRING descriptor describing device's SN

    uint8_t     bNumConfigurations; // Number of possible configurations
} USBDeviceDescriptor_t;

/**
 * @brief Endpoint descriptor
 */
typedef struct USBEndpointDescriptor {
    uint8_t     bLength;            // Size of this descriptor in bytes
    uint8_t     bDescriptorType;    // USB_DESC_ENDPOINT

    uint8_t     bEndpointAddress;   // Address of the endpoint on the USB device (bit 7 is direction, bits 0-3 are the endpoint number)
    uint8_t     bmAttributes;       // Endpoint attributes
    uint16_t    wMaxPacketSize;     // Maximum packet size
    uint8_t     bInterval;          // Interval for polling a device during a transfer
} USBEndpointDescriptor_t;


#endif