/**
 * @file source/kmods/usb/dev.c
 * @brief USB device handler
 * 
 * This file handles initialization and requests for USB devices
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include "dev.h"
#include <kernel/list.h>
#include <libk_reduced/sleep.h>

static list_t *usb_device_list;
static int nextUSBAddress;

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
    serialPrintf("[module usb] Initializing device (dev->controller = 0x%x)...\n", dev->controller);
    USBDeviceDescriptor_t device_desc;
    if (!usb_request(dev, USB_RT_D2H | USB_RT_STANDARD | USB_RT_DEV,
                    USB_REQ_GET_DESC, (USB_DESC_DEVICE << 8) | 0, 0, 8, &device_desc)) 
    {
        // The request did not succeed
        return 0;
    }

    dev->maxPacketSize = device_desc.maxPacketSize;

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