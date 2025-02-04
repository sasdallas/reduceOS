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
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
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
#include <kernel/ide_ata.h>



// ahci_getPortType(ahci_hba_port_t *port) - Get the port's type
static AHCI_DEVTYPE ahci_getPortType(ahci_hba_port_t *port) {
    uint32_t ssts = port->ssts;

    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT) return AHCI_DEV_NONE;
    if (ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NONE;

    // Check the port's signature
    switch (port->sig) {
        case AHCI_PORTSIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case AHCI_PORTSIG_SEMB:
            return AHCI_DEV_SEMB;
        case AHCI_PORTSIG_PM:
            return AHCI_DEV_PM;
        case AHCI_DEV_NONE:
            return AHCI_DEV_NONE;
        default:
            return AHCI_DEV_SATA;
    }
}

// ahci_probePorts(ahci_hba_mem_t *abar) - Probes the ports at ABAR
void ahci_probePorts(ahci_hba_mem_t *abar) {
    uint32_t pi = abar->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & 1) {
            int dt = ahci_getPortType(&abar->ports[i]);
            switch (dt) {
                case AHCI_DEV_SATA:
                    serialPrintf("[module ahci] SATA device found at port %d\n", i);
                    break;
                case AHCI_DEV_SATAPI:
                    serialPrintf("[module ahci] SATAPI device found at port %d\n", i);
                    break;
                case AHCI_DEV_SEMB:
                    serialPrintf("[module ahci] SEMB device found at port %d\n", i);
                    break;
                case AHCI_DEV_PM:
                    serialPrintf("[module ahci] PM device found at port %d\n", i);
                    break;
                default:
                    break;
            }
        }
    }
}


static int found_ahci = 0;      // TEMPORARY PATCH!
                                // The bug here most likely lies in pciScan but I'd rather stabilize the OS to a "nominal" point.

static void find_ahci(uint32_t device, uint16_t vendorID, uint16_t deviceID, void *extra) {
    if (found_ahci == 1) return;
    if (pciGetType(device) != 0x0106) return; // We're looking for a device with the "Mass Storage, SATA controller" type
    if (pciConfigReadField(device, PCI_OFFSET_PROGIF, 1) != 0x01) return; // Not an AHCI device

    serialPrintf("[module ahci] PCI device found with venid 0x%x and devid 0x%x\n", vendorID, deviceID);
    printf("Found AHCI device with vendor ID 0x%x device ID 0x%x\n", vendorID, deviceID);
    found_ahci = 1;

    return;


    // We're going to write a value to the PCI command register
    uint16_t command_reg = pciConfigReadField(device, PCI_OFFSET_COMMAND, 2);
    command_reg |= (1 << 2);    // Turn on the bus master bit 
    command_reg |= (1 << 1);    // Turn on the memory space
    // command_reg ^= (1 << 10);
    pciConfigWriteField(device, PCI_OFFSET_COMMAND, 2, command_reg); 

    // Print out some debug information
    serialPrintf("[module ahci] PCI interrupt line = %d\n", pciGetInterrupt(device));

    // Use the memory subsystem to map regions
    uint32_t addr = pciConfigReadField(device, PCI_OFFSET_BAR5, 4) & 0xFFFFFFF0;
    mem_mapAddress(NULL, addr, addr);
    mem_mapAddress(NULL, addr + 0x1000, addr + 0x1000);

    ahci_hba_mem_t *mem = (ahci_hba_mem_t*)addr;

    // Read the AHCI version (debug)
    uint32_t ahciVersion = mem->vs;
    serialPrintf("[module ahci] Controller version %d.%d%d\n", (ahciVersion >> 16) & 0xFFF, (ahciVersion >> 8) & 0xFF, (ahciVersion) & 0xFF);

    // Broken
    ahci_probePorts(mem);
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
