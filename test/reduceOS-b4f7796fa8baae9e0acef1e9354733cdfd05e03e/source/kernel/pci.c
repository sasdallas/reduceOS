// ===================================================================
// pci.c - Handles the Peripheral Component Interconnect (PCI) bus
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

// For more information on this topic, check https://wiki.osdev.org/PCI

#include "include/pci.h" // Main header file


bool isPCIInitialized = false; // In case a function is ever called before initPCI is called, it knows to automatically call initPCI.
static uint32_t pciAdapters[MAX_BUS][MAX_SLOTS];

// pciConfigRead(uint32_t bus, uint32_t slot, uint32_t offset) - Handles reading a PCI configuration.
uint16_t pciConfigRead(uint32_t bus, uint32_t slot, uint32_t offset) {
    // pci.h gives some vague descriptions of CONFIG_ADDR and CONFIG_DATA - here's a little more detail.
    // To get PCI config data, we need to first send CONFIG_ADDR (0xCF8) a proper address of the PCI component we want to access (composed of a bus, slot, offset, and 0x80000000)
    // This tells CONFIG_DATA where to read the data from. We send a read request to CONFIG_DATA, and it returns the data we want.

    uint32_t data = -1; // Final data will be stored here
    uint32_t address = (uint32_t) (0x80000000 | (bus << 16) | (slot << 11) | offset); // Address to send to CONFIG_ADDR

    // Send the address...
    outportl(CONFIG_ADDR, address);


    // Read the data...
    data = inportl(CONFIG_DATA);

    return data;
}

// pciConfigWrite(uint32_t bus, uint32_t slot, uint32_t offset, uint32_t value) - Write to a PCI configuration.
void pciConfigWrite(uint32_t bus, uint32_t slot, uint32_t offset, uint32_t value) {
    if (isPCIInitialized) {
        outportl(CONFIG_ADDR, bus);
        outportl(CONFIG_ADDR, 0xF0);
        outportl(0xC000 | (slot << 8) | offset, value); // 0xC000 is the start of a PCI IO configuration
    } else {
        // initPCI has not been called yet.
        outportl(CONFIG_ADDR, (0x80000000 | (bus << 16) | (slot << 11) | offset));
        outportl(CONFIG_DATA, value);
    }
}


// pciGetSize(uint32_t bus, uint32_t slot, uint32_t r) - A small function used to get the size of a PCI device at bus "bus" and slot "slot" (index r)
uint32_t pciGetSize(uint32_t bus, uint32_t slot, uint32_t r) {
    uint32_t tmp;
    uint32_t ret;

    // First, backup the OG value.
    tmp = pciConfigRead(bus, slot, 0x10 + r*4);

    // Determine the size...
    pciConfigWrite(bus, slot, 0x10 + r*4, 0xFFFFFFFF);
    ret = pciConfigRead(bus, slot, 0x10 + r*4) + 1;

    // Restore OG value
    pciConfigWrite(bus, slot, 0x10 + r*4, tmp);

    // Return size
    return ret;
}

// getPCIDeviceInfo(uint32_t vendor, uint32_t deviceID, uint32_t base, pci_info *info) - Returns/sets the proper values corresponding to the PCI device found in base.
void getPCIDeviceInfo(uint32_t vendor, uint32_t deviceID, uint32_t base, pci_info *info) {


    if (!isPCIInitialized) initPCI();

    for (uint32_t bus = 0; bus < MAX_BUS; bus++) {
        for (uint32_t slot = 0; slot < MAX_SLOTS; slot++) {
            if (pciAdapters[bus][slot] != -1) {
                // We now know that there is a PCI device here. Check if the vendor and device ID match the ones provided.
                if (((pciAdapters[bus][slot] & 0xFFFF) == vendor) && (pciAdapters[bus][slot] & 0xFFFF0000 >> 16) == deviceID) {
                    // Cool, they do! Update slot and bus in info to the proper values.
                    info->slot = slot;
                    info->bus = bus;

                    // Now fill in the rest of the information.
                    for (uint32_t i = 0; i < 6; i++) {
                        info->base[i] = pciConfigRead(bus, slot, 0x10 + i*4) & 0xFFFFFFFC; // 0x10 is the configuration base IO address
                        info->type[i] = pciConfigRead(bus, slot, 0x10 + i*4) & 0x1;
                        info->size[i] = (info->base[i]) ? pciGetSize(bus, slot, i) : 0;
                    }

                    info->irq = pciConfigRead(bus, slot, 0x3C) & 0xFF; // 0x3C is the configuration interrupt

                    if (base) { if (!(info->base[0] == base)) continue; }
                    return 0;
                }
            }
        }
    }

    return -1; // Unable to get device information for some reason.
}


void initPCI() {
    // Check all PCI adapters.

    for (uint32_t bus = 0; bus < MAX_BUS; bus++) {
        for (uint32_t slot = 0; slot < MAX_SLOTS; slot++) {
            pciAdapters[bus][slot] = pciConfigRead(bus, slot, 0x00); // 0x00 is the configuration ID
        }
    }

    // Make sure the functions know initialization has completed.
    isPCIInitialized = true;
    printf("PCI handler initialized.\n");
}


void printPCIInfo() {
    if (!isPCIInitialized) initPCI();

    uint32_t deviceCounter = 0;

    for (uint32_t bus = 0; bus < MAX_BUS; bus++) {
        for (uint32_t slot = 0; slot < MAX_SLOTS; slot++) {
            if (pciAdapters[bus][slot] != -1 && (pciAdapters[bus][slot] & 0xFFFF) != 0xFFFF && (pciAdapters[bus][slot] & 0xFFFF0000) >> 16 != 0x0) {
                deviceCounter++;
                
                printf("%d) Vendor ID: 0x%x; Device ID: 0x%x\n", deviceCounter, pciAdapters[bus][slot] & 0xFFFF, (pciAdapters[bus][slot] & 0xFFFF0000) >> 16);
            }
        }
    }

    if (deviceCounter == 0) printf("No PCI devices found.\n");
}