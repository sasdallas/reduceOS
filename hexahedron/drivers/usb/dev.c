/**
 * @file hexahedron/drivers/usb/dev.c
 * @brief USB device handler
 * 
 * Handles initialization and requests between controllers and devices
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/usb/usb.h>
#include <kernel/drivers/usb/dev.h>
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>
#include <string.h>

/* Log method */
#define LOG(status, ...) dprintf_module(status, "USB:DEV", __VA_ARGS__)


/**
 * @brief Create a new USB device structure for initialization
 * @param controller The controller
 * @param port The device port
 * @param speed The device speed
 * 
 * @param control The HC control request method
 */
USBDevice_t *usb_createDevice(USBController_t *controller, uint32_t port, int speed, hc_control_t control) {
    USBDevice_t *dev = kmalloc(sizeof(USBDevice_t));
    memset(dev, 0, sizeof(USBDevice_t));

    dev->c = controller;
    dev->control = control;
    dev->port = port;
    dev->speed = speed;

    // By default, during initialization the USB device expects to receive an address of 0
    dev->address = 0;

    return dev;
}

/**
 * @brief Initialize a USB device and assign to the USB controller's list of devices
 * @param dev The device to initialize
 * 
 * @returns Negative value on failure and 0 on success
 */
int usb_initializeDevice(USBDevice_t *dev) {
    LOG(DEBUG, "Initializing USB device on port 0x%x...\n", dev->port);


    // Get first few bytes of the device descriptor
    // TODO: Bochs requests that this have a size equal the mps of a device - implement this
    if (usb_requestDevice(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                    USB_REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0, dev->mps, &dev->device_desc) != USB_TRANSFER_SUCCESS) 
    {
        // The request did not succeed
        LOG(ERR, "USB_REQ_GET_DESC did not succeed\n");
        return -1;
    }

    // Set the maximum packet size
    dev->mps = dev->device_desc.bMaxPacketSize0;

    // Get an address for it (base should be 0x0)
    uint32_t address = dev->c->last_address;
    dev->c->last_address++;

    // Request it to set that address
    if (usb_requestDevice(dev, USB_RT_H2D | USB_RT_STANDARD | USB_RT_DEV,
                    USB_REQ_SET_ADDR, address, 0, 0, NULL) != USB_TRANSFER_SUCCESS)
    {
        // The request did not succeed
        LOG(ERR, "Device initialization failed - USB_REQ_SET_ADDR 0x%x did not succeed\n", address);
        return -1;
    }

    dev->address = address; // Yes, this is required to be set after SET_ADDR, else the device panicks.

    // Now we can read the whole descriptor
    if (usb_requestDevice(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0, sizeof(USBDeviceDescriptor_t), &dev->device_desc) != USB_TRANSFER_SUCCESS) 
    {
        // The request did not succeed
        LOG(ERR, "Device initialization failed - failed to read full descriptor\n");
        return -1;
    }


    LOG(DEBUG, "USB Device: Version %d.%d, VID 0x%04x, PID 0x%04x PROTOCOL 0x%04x\n", dev->device_desc.bcdUSB >> 8, (dev->device_desc.bcdUSB >> 4) & 0xF, dev->device_desc.idVendor, dev->device_desc.idProduct, dev->device_desc.bDeviceProtocol);

    // Add it to the device list of the controller
    list_append(dev->c->devices, dev);

    // Read the language IDs supported by this device
    uint8_t lang_length;

    // We need bLength so only read 1 byte
    if (usb_requestDevice(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV, 
                USB_REQ_GET_DESC, (USB_DESC_STRING << 8) | 0, 0, 1, &lang_length) != USB_TRANSFER_SUCCESS) 
    {
        LOG(ERR, "Device initialization failed - could not get language codes\n");
        return -1;
    }

    // Now we can read the full descriptor
    dev->langs = kmalloc(lang_length);

    if (usb_requestDevice(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_GET_DESC, (USB_DESC_STRING << 8) | 0, 0, lang_length, dev->langs) != USB_TRANSFER_SUCCESS)
    {
        LOG(ERR, "Device initialization failed - could not get all language codes\n");
        return -1;
    }

    for (int i = 0; i < (dev->langs->bLength - 2) / 2; i++) {
        LOG(DEBUG, "Supports language code: 0x%02x\n", dev->langs->wLangID[i]);
        if (dev->langs->wLangID[i] & USB_LANGID_ENGLISH) {
            // We don't support Unicode yet
            dev->chosen_language = dev->langs->wLangID[i];
        }
    }

    // Done! We've got the device language codes. Now we've unlocked usb_getStringIndex
    char *product_str = usb_getStringIndex(dev, dev->device_desc.iProduct, dev->chosen_language);
    char *vendor_str = usb_getStringIndex(dev, dev->device_desc.iManufacturer, dev->chosen_language);  
    char *serial_number = usb_getStringIndex(dev, dev->device_desc.iSerialNumber, dev->chosen_language);

    LOG(INFO, "Initialized USB device '%s' from '%s' (SN %s)\n", product_str, vendor_str, serial_number);

    return 0;
}


/**
 * @brief Read a string from the USB device
 * @param dev The device to read the string from
 * @param idx The index of the string to read
 * @param lang The language code
 * @returns ASCII string (converted from the normal unicode)
 */
char *usb_getStringIndex(USBDevice_t *device, int idx, uint16_t lang) {
    if (idx == 0) {
        // String index #0 is reserved for languages
        LOG(WARN, "Cannot read string index #0 - reserved for languages\n");
        return NULL;
    }

    // Request the descriptor
    uint8_t bLength;

    if (usb_requestDevice(device, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_GET_DESC, (USB_DESC_STRING << 8) | idx, lang, 1, &bLength) != USB_TRANSFER_SUCCESS)
    {
        LOG(WARN, "Failed to get string index %i for device\n", idx);
        return NULL;
    }


    // Now read the full descriptor
    USBStringDescriptor_t *desc = kmalloc(bLength);

    if (usb_requestDevice(device, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_GET_DESC, (USB_DESC_STRING << 8) | idx, lang, bLength, desc) != USB_TRANSFER_SUCCESS)
    {
        LOG(WARN, "Failed to get string index %i for device\n", idx);
        return NULL;
    }

    // Convert it to ASCII
    char *string_output = kmalloc(((bLength - 2) / 2) + 1); // Subtract 2 for the 2 extra descriptor bytes and divide by 2 for short -> byte conversion, with one extra for null termination
    memset(string_output, 0, ((bLength - 2) / 2) + 1);

    for (int i = 0; i < (bLength - 2); i += 2) {
        string_output[i/2] = desc->bString[i];
    }

    return string_output;
}

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
 * @returns The request status 
 */
int usb_requestDevice(USBDevice_t *device, uintptr_t type, uintptr_t request, uintptr_t value, uintptr_t index, uintptr_t length, void *data) {
    if (!device) return -1;

    // Create a new device request
    USBDeviceRequest_t *req = kmalloc(sizeof(USBDeviceRequest_t));
    req->bmRequestType = type;
    req->bRequest = request;
    req->wIndex = index;
    req->wValue = value;
    req->wLength = length;

    // Create a new transfer
    USBTransfer_t *transfer = kmalloc(sizeof(USBTransfer_t));
    transfer->req = req;
    transfer->endpoint = 0;      // TODO: Allow custom endpoints - CONTROL requests don't necessary have to come from the DCP
    transfer->status = USB_TRANSFER_IN_PROGRESS;
    transfer->length = length;
    transfer->data = data;
    
    // Now send the device control request
    if (device->control) device->control(device->c, device, transfer);

    // Cleanup and return whether the transfer was successful
    int status = transfer->status;
    kfree(transfer);
    kfree(req);
    return status;
}

