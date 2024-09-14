/**
 * @file source/kmods/usb/dev.c
 * @brief USB device handler
 * 
 * This file handles initialization and requests for USB devices
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include "dev.h"
#include <kernel/list.h>
#include <libk_reduced/sleep.h>

static list_t *usb_device_list;
static int nextUSBAddress = 0;

/**
 * @brief Creates a USBDevice_t structure
 */
USBDevice_t *usb_createDevice() {
    // This is pretty simple. Allocate memory, zero it, setup some values, bing bang boom.
    USBDevice_t *dev = kmalloc(sizeof(USBDevice_t));
    memset(dev, 0, sizeof(USBDevice_t));
    list_insert(usb_device_list, dev);
    return dev;
}

/**
 * @brief Get the USB device list
 */
list_t *usb_getUSBDeviceList() {
    return usb_device_list;
}


/**
 * @brief Returns the supported languages by a device
 * @param dev The USB device
 * @param languages Output
 */
int usb_getLanguages(USBDevice_t *dev, uint16_t *languages) {
    languages[0] = 0; // Prevent faults

    
}

/**
 * @brief Initialize a USB device
 * @param dev USB device
 */
int usb_initDevice(USBDevice_t *dev) {
    serialPrintf("[module usb] Initializing device (speed = 0x%x)...\n", dev->speed);
    USBDeviceDescriptor_t device_desc;

    // it's buggy if we mess with anything that's not low speed
    uint8_t mps_old = dev->maxPacketSize;

    switch (dev->speed) {
        case USB_LOW_SPEED:
            dev->maxPacketSize = 8;
            break;
        case USB_HIGH_SPEED:
        case USB_FULL_SPEED:
            dev->maxPacketSize = 64;
            break;
        default:
            // super speed?
            dev->maxPacketSize = 512;
            break;
    };


    if (!usb_request(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                    USB_REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0, 8, &device_desc)) 
    {
        // The request did not succeed
        return 0;
    }

    serialPrintf("[module usb] Request USB_REQ_GET_DESC completed. Device descriptor:\n");
    serialPrintf("[module usb]\tDevice descriptor - length 0x%x type 0x%x spec 0x%x devclass 0x%x devsubclass 0x%x protocol 0x%x mps 0x%x\n",
                device_desc.length, device_desc.type, device_desc.spec, device_desc.devclass, device_desc.devsubclass, device_desc.protocol, device_desc.maxPacketSize);

    dev->maxPacketSize = device_desc.maxPacketSize;

    uhci_resetPort((uhci_t*)dev->controller, dev->port);    

    
    // Setup the address (request it)
    uint32_t addr = nextUSBAddress++;
    if (!usb_request(dev, USB_RT_H2D | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_SET_ADDR, addr, 0, 0, 0)) 
    {
        // The request did not succeed
        return 0;
    }

    dev->addr = addr;

    // Wait for device to do thingy
    sleep(2);
    

    

    // Now let's read in the entire descriptor
    if (!usb_request(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                USB_REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0, sizeof(USBDeviceDescriptor_t), &device_desc)) 
    {
        return 0;
    }

    // Debug
    serialPrintf(" USB Device: Version %d.%d, VID 0x%04x, PID=%04x, available configs = %d\n",
            device_desc.spec >> 8, (device_desc.spec >> 4) & 0xF,
                device_desc.vend_id, device_desc.prod_id, device_desc.num_confs);
     
    
}

/**
 * @brief Send a request to control the device
 * @param dev The device
 * @param type Type of device request
 * @param request The actual request put into USBDeviceRequest_t
 * @param index The index of the device request
 * @param length The length of the device request
 * @param data The data of the request
 */
int usb_request(USBDevice_t *dev, uint32_t type, uint32_t request,
                uint32_t value, uint32_t index, uint32_t length, void *data)
{
    // Allocate memory and setup both the device request and transfer
    USBDeviceRequest_t *req = kmalloc(sizeof(USBDeviceRequest_t));
    req->type = type;
    req->request = request;
    req->value = value;
    req->index = index;
    req->length = length;

    USBTransfer_t *t = kmalloc(sizeof(USBTransfer_t));
    t->endp = 0;
    t->req = req;
    t->data = data;
    t->length = length;
    t->complete = 0;
    t->success = 0;

    dev->control(dev, t);

    // Now cleanup and return whether the transfer was succesful
    int success = t->success;
    kfree(t);
    kfree(req);
    return success;
}


/**
 * @brief Initialize the device handler
 */
void usb_DevInit() {
    usb_device_list = list_create();
}