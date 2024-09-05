/**
 * @file source/kmods/usb/usb.c
 * @brief The main file for the USB (Universal Serial Bus) driver for the reduceOS kernel.
 * 
 * This driver handles USB and supported controllers that are in it.
 * It is all packaged into this driver, such as UHCI/OHCI, as well as peripheral devices.
 * @todo At some point I would like to integrate this or a way to read function from this into kernel, making it easier (if not downright impossible) to write USB peripheral drivers 
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "uhci.h"
#include <kernel/mod.h>
#include <kernel/pit.h>
#include <kernel/pci.h>

static void find_usb(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) {
    if (pciGetType(device) != 0x0C03 && pciConfigReadField(device, PCI_OFFSET_PROGIF, 4) != 0x00) return;

    serialPrintf("[module usb] Found a UHCI controller\n");
    uhci_init(device);

}

int usb_init() {
    pciScan(find_usb, -1, NULL);

    return 0;
}

void usb_deinit() {

}

struct Metadata data = {
    .name = "USB driver",
    .description = "reduceOS Universal Serial Bus driver",
    .init = usb_init,
    .deinit = usb_deinit
};

