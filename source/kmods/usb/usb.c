/**
 * @file source/kmods/usb/usb.c
 * @brief The main file for the USB (Universal Serial Bus) driver for the reduceOS kernel.
 * 
 * This driver handles USB and supported controllers that are in it.
 * It is all packaged into this driver, such as UHCI/OHCI, as well as peripheral devices.
 * @todo At some point I would like to integrate this or a way to read function from this into kernel, making it easier (if not downright impossible) to write USB peripheral drivers 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include "usb.h"
#include <kernel/mod.h>
#include <kernel/clock.h>
#include <kernel/pci.h>

static list_t *usb_controllers;

static void find_usb(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) {
    if (pciGetType(device) == 0x0C03 && pciConfigReadField(device, PCI_OFFSET_PROGIF, 1) == 0x00) {
        serialPrintf("[module usb] Found a UHCI controller\n");
        uhci_init(device);
    }

    if (pciGetType(device) == 0x0C03 && pciConfigReadField(device, PCI_OFFSET_PROGIF, 1) == 0x20) {
        serialPrintf("[module usb] Found an EHCI controller\n");
    }
}

/**
 * @brief Add a controller to the USB controller list
 */
void usb_addController(USBController_t *controller) {
    list_insert(usb_controllers, controller);
}


/**
 * @brief USB poll method
 */
void usb_poll(unsigned long seconds, unsigned long subseconds) {

    foreach(hcnode, usb_controllers) {
        USBController_t *hc = (USBController_t*)hcnode->value;
        hc->poll(hc);
    }

    list_t *usblist = usb_getUSBDeviceList();
    foreach(devnode, usblist) {
        USBDevice_t *dev = (USBDevice_t*)devnode->value;
        if (dev->poll) {
            dev->poll(dev);
        }
    }
}


/**
 * @brief Initialize the USB module
 */

int usb_init() {

    serialPrintf("[module usb] Disabled for now!\n");
    return 0;

    usb_DevInit();                      // Initialize the device list
    usb_controllers = list_create();    // Initialize the controllers list
    clock_registerCallback(&usb_poll);  // Register poll method
    pciScan(find_usb, -1, NULL);        // Scan for USB controllers

    return 0;
}


/**
 * @brief Deinitialize the USB module
 */
int usb_deinit() {
    return 0;
}

struct Metadata data = {
    .name = "USB Driver",
    .description = "reduceOS Universal Serial Bus driver",
    .init = usb_init,
    .deinit = usb_deinit
};

