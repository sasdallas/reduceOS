// ===================================================================
// pci.c - Handles the Peripheral Component Interconnect (PCI) bus
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

// For more information on this topic, check https://wiki.osdev.org/PCI

#include "include/pci.h" // Main header file

// TODO: Implement support for different header types, we always assume header type 0x0.

bool isPCIInitialized = false; // In case a function is ever called before initPCI is called, it knows to automatically call initPCI.

pciDevice *pciDevices[32];
uint32_t dev_idx = 0;

pciDriver **pciDrivers = 0;
uint32_t drv_idx = 0;

// pciConfigRead(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) - Handles reading a PCI configuration.
uint32_t pciConfigRead(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    // pci.h gives some vague descriptions of CONFIG_ADDR and CONFIG_DATA - here's a little more detail.
    // To get PCI config data, we need to first send CONFIG_ADDR (0xCF8) a proper address of the PCI component we want to access (composed of a bus, slot, offset, and 0x80000000)
    // This tells CONFIG_DATA where to read the data from. We send a read request to CONFIG_DATA, and it returns the data we want.

    // First, convert bus, slot, and func to uint64_t type
    uint64_t addr;
    uint64_t long_bus = (uint64_t)bus;
    uint64_t long_slot = (uint64_t)slot;
    uint64_t long_func = (uint64_t)func;
    
    // Now, calculate and send our PCI device addr.
    addr = (uint64_t)((long_bus << 16) | (long_slot << 11) | (long_func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outportl(CONFIG_ADDR, addr);

    uint32_t output = (uint32_t)((inportl(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return output;
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



uint16_t pciGetVendorID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)pciConfigRead(bus, device, function, OFFSET_VENDORID);
}



uint16_t pciGetDeviceID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)pciConfigRead(bus, device, function, OFFSET_DEVICEID);
}

uint16_t pciGetClassID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)(((uint32_t)pciConfigRead(bus, device, function, OFFSET_CLASSID)) & ~0x00FF) >> 8;
}

uint16_t pciGetSubClassID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)(((uint32_t)pciConfigRead(bus, device, function, OFFSET_SUBCLASSID)) & ~0xFF00);
}

void pciProbeForDevices() {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t slot = 0; slot < 32; slot++) {
            for (uint32_t func = 0; func < 8; func++) {
                uint16_t vendor = pciGetVendorID(bus, slot, func);
                if (vendor == 0xFFFF) continue; // Not a PCI device
                uint16_t deviceID = pciGetDeviceID(bus, slot, func);


                serialPrintf("pciProbeForDevices: Found PCI device (function = 0x%x, vendor ID = 0x%x, device ID = 0x%x)\n", func, vendor, deviceID);

                pciDevice *dev = (pciDevice*)kmalloc(sizeof(pciDevice));
                dev->device = deviceID;
                dev->driver = 0;
                dev->func = func;
                dev->vendor = vendor;
                dev->bus = bus;
                dev->slot = slot;


                pciDevices[dev_idx] = dev;
                dev_idx++;
            }
        }
    }
}


void initPCI() {
    if (isPCIInitialized) return; // Stupid users

    // Allocate memory for devices and drivers
    pciDrivers = (pciDriver**)kmalloc(32 * sizeof(pciDriver));

    // Probe PCI devices
    pciProbeForDevices();

    // Make sure the functions know initialization has completed.
    isPCIInitialized = true;
    printf("PCI handler initialized.\n");
}

int getDevTableId(uint32_t deviceID, uint32_t vendorID) {
    int id = -1;
    for (int i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (PciDevTable[i].DevId == deviceID && PciDevTable[i].VenId == vendorID) {
            id = i;
            break;
        }
    }
    
    return id;
}

int getClassIdType(uint16_t classID, uint16_t subclassID) {
    int id = -1;

    for (int i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].BaseClass == classID && PciClassCodeTable[i].SubClass == subclassID) {
            id = i;
            break;
        }
    }

    return id;
}

void printPCIInfo() {

    for (int i = 0; i < dev_idx; i++) {
        
        // Get the class type
        uint16_t classID = pciGetClassID(pciDevices[i]->bus, pciDevices[i]->slot, pciDevices[i]->func);
        uint16_t subclassID = pciGetSubClassID(pciDevices[i]->bus, pciDevices[i]->slot, pciDevices[i]->func);
        int classTypeId = getClassIdType(classID, subclassID);        

        int id = getDevTableId(pciDevices[i]->device, pciDevices[i]->vendor);
        if (id != -1 && classTypeId != -1) {
            if (PciClassCodeTable[classTypeId].ProgDesc != "") {
                printf("%i) %s %s (%s - %s - %s)\n", i, PciDevTable[id].Chip, PciDevTable[i].ChipDesc, PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc, PciClassCodeTable[classTypeId].ProgDesc);
            } else {
                printf("%i) %s %s (%s - %s)\n", i, PciDevTable[id].Chip, PciDevTable[i].ChipDesc, PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc);
            }
        } else if (classTypeId != -1) {
            // We don't know the exact device type - that's ok, just print Unknown and the class type
            if (PciClassCodeTable[classTypeId].ProgDesc != "") {
                printf("%i) Unknown Device (%s - %s - %s)\n", i,  PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc, PciClassCodeTable[classTypeId].ProgDesc);
            } else {
                printf("%i) Unknown Device (%s - %s)\n", i, PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc);
            }
        } else {
            // We don't know anything.
            printf("%i) Unknown Device (Unknown Class Type)\n", i);
        }

        printf("\tVendor ID: 0x%x, Device ID: 0x%x\n", pciDevices[i]->vendor, pciDevices[i]->device);
        // pciGetStatus(pciDevices[i]->bus, pciDevices[i]->slot, pciDevices[i]->func);
    
    }

}