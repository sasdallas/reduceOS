/**
 * @file source/kmods/ahci/ahci.c
 * @brief The main file for the AHCI (Advanced Host Controller Interface) module on the reduceOS kernel.
 * 
 * This driver handles AHCI, a specification that Intel created created.
 * AHCI is used to handle SATA devices - physical controllers are present that can be interacted with.
 * 
 * An AHCI controller is used as a "data movement engine between system memory and SATA devices" (source: OSdev wiki)
 * A controller will encapsulate SATA devices, and exposes a simple PCI interface.
 * A maximum of 32 ports are supported.
 * 
 * AHCI is meant to be the a the successor to IDE, which is integrated right into the reduceOS kernel.
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include "ahci.h"

#include <libk_reduced/errno.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/spinlock.h>
#include <kernel/vfs.h>
#include <kernel/hal.h>
#include <kernel/mod.h>
#include <kernel/pci.h>
#include <kernel/vmm.h>


// AHCI uses memoryspace
static uint32_t ahci_readmem(uintptr_t base, intptr_t offset) {
    volatile uint32_t *data = (volatile uint32_t *)(base + offset);
    return *data;
}

static uint32_t ahci_writemem(uintptr_t base, intptr_t offset, uint32_t value) {
    volatile uint32_t *data = (volatile uint32_t *)(base + offset);
    *data = value;
}



static void find_ahci(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) {
    if (pciGetType(device) != 0x0106) return; // We're looking for a device with the "Mass Storage, SATA controller" type
    if (pciConfigReadField(device, PCI_OFFSET_PROGIF, 1) != 0x01) return; // Not an AHCI device

    serialPrintf("[module ahci] PCI device found with venid 0x%x and devid 0x%x\n", vendorID, deviceID);

    // We're going to write a value to the PCI command register
    uint16_t command_reg = pciConfigReadField(device, PCI_OFFSET_COMMAND, 2);
    command_reg |= (1 << 2);    // Turn on the bus master bit 
    command_reg |= (1 << 1);    // Turn on the memory space
    command_reg ^= (1 << 10);   // This is useless, it's but the interrupt disable bit will either be set/unset. This doesn't change it.
    pciConfigWriteField(device, PCI_OFFSET_COMMAND, 2, command_reg); 

    // Print out some debug information
    serialPrintf("[module ahci] PCI interrupt line = %d\n", pciGetInterrupt(device));
    serialPrintf("[module ahci] BAR5 = %#x\n", pciConfigReadField(device, PCI_OFFSET_BAR5, 4));

    // Use the VMM to map regions
    uint32_t addr = pciConfigReadField(device, PCI_OFFSET_BAR5, 4) & 0xFFFFFFF0;
    vmm_allocateRegionFlags(addr, addr, 0x2000, 1, 1, 0); // do we need usermode? IRQ handler should deal with this

    // Read MMIO, we'll use regular address
    uint32_t enabledPorts = ahci_readmem(addr, 0x0C);
    serialPrintf("[module ahci] Enabled ports = %#x\n", enabledPorts);

    // Read the AHCI version (debug)
    uint32_t ahciVersion = ahci_readmem(addr, 0x10);
    serialPrintf("[module ahci] Controller version %d.%d%d\n", (ahciVersion >> 16) & 0xFFF, (ahciVersion >> 8) & 0xFF, (ahciVersion) & 0xFF);

    int offset = 0x100;
    for (int port = 0; port < 32; port++) {
        if (enabledPorts & (1UL << port)) {
            uint32_t portSig = ahci_readmem(addr, offset + 0x24);
            uint32_t portStatus = ahci_readmem(addr, offset + 0x28);

            serialPrintf("[module ahci] port %d: status     = %#x\n", port, portStatus);
            serialPrintf("[module ahci] port %d: sig        = %#x\n", port, portSig);

            switch (portSig) {
                case AHCI_PORTSIG_ATAPI:
                    serialPrintf("[module ahci]\tATAPI (CD, DVD)\n");
                    break;
                case AHCI_PORTSIG_HDD:
                    serialPrintf("[module ahci]\tHard disk\n");
                    break;
                case AHCI_PORTSIG_NONE:
                    serialPrintf("[module ahci]\tNo devices\n");
                    break;
                default:
                    serialPrintf("[module ahci]\t???\n");
                    break;
            }
        }

        offset += 0x80;
 
    }
}


int ahci_init() {
    pciScan(find_ahci, -1, NULL);

    return 0;
}


void ahci_deinit() {

}

struct Metadata data = {
    .name = "AHCI Driver",
    .description = "Driver for the Intel AHCI standard",
    .init = ahci_init,
    .deinit = ahci_deinit
};
