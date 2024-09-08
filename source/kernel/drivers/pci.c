// ===================================================================
// pci.c - Handles the Peripheral Component Interconnect (PCI) bus
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.
// @warning This file is messy. I recommend you don't use it

// For more information on this topic, check https://wiki.osdev.org/PCI

#include <kernel/pci.h> // Main header file
#include <libk_reduced/stdio.h>

// TODO: Implement support for different header types, we always assume header type 0x0.

bool isPCIInitialized = false; // In case a function is ever called before initPCI is called, it knows to automatically call initPCI.

pciDevice *pciDevices[32];
uint32_t dev_idx = 0;


// pciConfigRead(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) - Handles reading a PCI configuration.
uint32_t pciConfigRead(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset) {
    // pci.h gives some vague descriptions of PCI_CONFIG_ADDR and PCI_CONFIG_DATA - here's a little more detail.
    // To get PCI config data, we need to first send PCI_CONFIG_ADDR (0xCF8) a proper address of the PCI component we want to access (composed of a bus, slot, offset, and 0x80000000)
    // This tells PCI_CONFIG_DATA where to read the data from. We send a read request to PCI_CONFIG_DATA, and it returns the data we want.

    // First, convert bus, slot, and func to uint64_t type
    uint64_t addr;
    uint64_t long_bus = (uint64_t)bus;
    uint64_t long_slot = (uint64_t)slot;
    uint64_t long_func = (uint64_t)func;
    
    // Now, calculate and send our PCI device addr.
    addr = (uint64_t)((long_bus << 16) | (long_slot << 11) | (long_func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outportl(PCI_CONFIG_ADDR, addr);

    uint32_t output = (uint32_t)((inportl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return output;
}

// pciConfigWrite(uint32_t bus, uint32_t slot, uint32_t offset, uint32_t value) - Write to a PCI configuration.
void pciConfigWrite(uint32_t bus, uint32_t slot, uint32_t offset, uint32_t value) {
    if (isPCIInitialized) {
        outportl(PCI_CONFIG_ADDR, bus);
        outportl(PCI_CONFIG_ADDR, 0xF0);
        outportl(0xC000 | (slot << 8) | offset, value); // 0xC000 is the start of a PCI IO configuration
    } else {
        // initPCI has not been called yet.
        outportl(PCI_CONFIG_ADDR, (0x80000000 | (bus << 16) | (slot << 11) | offset));
        outportl(PCI_CONFIG_DATA, value);
    }
}


// pciConfigReadField(uint32_t device, int field, int size) - A simplified version of pciConfigRead() that will take 3 parameters and read from a PCI device config space field.
// @todo Remove pciConfigRead
uint32_t pciConfigReadField(uint32_t device, int field, int size)  {
    outportl(PCI_CONFIG_ADDR, PCI_ADDR(device, field));

    if (size == 4) {
        return inportl(PCI_CONFIG_DATA);
    } else if (size == 2) {
        uint16_t r = inportw(PCI_CONFIG_DATA + (field & 2));
        return r;
    } else if (size == 1) {
        uint8_t r = inportb(PCI_CONFIG_DATA + (field & 3));
        return r;
    }

    return PCI_NONE; // Error
}

// pciConfigWriteField(uint32_t device, int field, int size, uint32_t value) - Write a value to the PCI device config space field
void pciConfigWriteField(uint32_t device, int field, int size, uint32_t value) {
    outportl(PCI_CONFIG_ADDR, PCI_ADDR(device, field));
    outportl(PCI_CONFIG_DATA, value);
}



/* PCI BARs - Handles base address registers */

pciBar_t *pciGetBAR(uint32_t device, int bar) {
    pciBar_t *out = kmalloc(sizeof(pciBar_t));

    // Read in the BAR itself first
    uint32_t bar_address = pciConfigReadField(device, bar, 4);

    // Now read in the BAR size
    pciConfigWriteField(device, bar, 4, 0xFFFFFFFF);
    uint32_t bar_size = pciConfigReadField(device, bar, 4);
    pciConfigWriteField(device, bar, 4, bar_address);

    // There are three types of BARs (that we need to handle) - 64-bit MMIO, I/O registers, and 32-bit MMIO
    // Each needs to be handled differently
    if (bar_address & PCI_BAR_MMIO64) {
        // This is a 64-bit value, we'll have to read in both.
        uint32_t bar_addressHigh = pciConfigReadField(device, bar + 1, 4);
        pciConfigWriteField(device, bar + 1, 4, 0xFFFFFFFF);
        uint32_t bar_sizeHigh = pciConfigReadField(device, bar + 1, 4);
        pciConfigWriteField(device, bar + 1, 4, bar_addressHigh);

        // Setup values
        #pragma GCC diagnostic ignored "-Wint-conversion" // BUG
        out->barAddress = (((uint64_t)bar_addressHigh << 32) | (bar_address & ~0xF)); // Remember to clear the lower 4 bits!
        out->size = ~(((uint64_t)bar_sizeHigh << 32) | (bar_size & ~0xF)) + 1;
        out->flags = bar_address & 0xF; // The lower 4-bits are the flags
    } else if (bar_address & PCI_BAR_IO) {
        // This is an I/O register (last two bits are flags)
        // GCC will not stop whining about an "invalid type conversion" without having a completely separate port...
        out->port =  (uint16_t)(bar_address & ~0x3);
        out->size = (uint16_t)(~(bar_size & ~0x3) + 1);
        out->flags = bar_address & 0x3;
    } else {
        // Assume this is a 32-bit MMIO BAR (lower 4 bits are flags)
        out->barAddress = (void*)(bar_address & ~0xF);
        out->size = ~(bar_size & ~0xF) + 1;
        out->flags = bar_address & 0xF;
    }

    return out;
}


/* PCI SCANNING - Scans the PCI buses for devices and calls the given function for each device */


// Helper function
uint16_t pciGetType(uint32_t dev) {
	return (pciConfigReadField(dev, PCI_OFFSET_CLASSID, 1) << 8) | pciConfigReadField(dev, PCI_OFFSET_SUBCLASSID, 1);
}


// (static) pciScanHit(pciFunction_t func, uint32_t device, void *extra) - Calls the function for the device
static void pciScanHit(pciFunction_t func, uint32_t device, void *extra) {
    int dev_vend = (int)pciConfigReadField(device, PCI_OFFSET_VENDORID, 2);
    int dev_dvid = (int)pciConfigReadField(device, PCI_OFFSET_DEVICEID, 2);
    func(device, dev_vend, dev_dvid, extra);
}

// pciScanFunc(pciFunction_t f, int type, int bus, int slot, int func, void *extra) - Scans the slots for a device
void pciScanFunc(pciFunction_t f, int type, int bus, int slot, int func, void *extra) {
    uint32_t dev = (uint32_t)((bus << 16) | (slot << 8) | func);

    if (type == -1 || type == pciGetType(dev)) {
        pciScanHit(f, dev, extra);
    }

    if (pciGetType(dev) == PCI_TYPE_BRIDGE) {
        // It's a bridge, do more work
        pciScanBus(f, type, pciConfigReadField(dev, PCI_SECONDARY_BUS, 1), extra);
    }
}


// pciScanSlot(pciFunction_t func, int type, int bus, int slot, void *extra) - Scans a slot for a device
void pciScanSlot(pciFunction_t func, int type, int bus, int slot, void *extra) {
    uint32_t device = (uint32_t)((bus << 16) | (slot << 8) | 0); 
    
    if (pciConfigReadField(device, PCI_OFFSET_VENDORID, 2) == PCI_NONE) {
        return;
    }   

    pciScanFunc(func, type, bus, slot, 0, extra);
    if (!pciConfigReadField(device, PCI_OFFSET_HEADERTYPE, 1)) return;

    for (int f = 0; f < 8; f++) {
        uint32_t device = (uint32_t)((bus << 16) | (slot << 8) | f);
        if (pciConfigReadField(device, PCI_OFFSET_VENDORID, 2) != PCI_NONE) {
            pciScanFunc(func, type, bus, slot, f, extra);
        }
    }
}


// pciScanBus(pciFunction_t func, int type, int bus, void *extra) - Scans each slot on the bus for a device
void pciScanBus(pciFunction_t func, int type, int bus, void *extra) {
    for (int slot = 0; slot < PCI_MAX_SLOTS; slot++) {
        pciScanSlot(func, type, bus, slot, extra);
    }
}


// pciScan(pciFunction_t func, int type, void *extra) - Scans the PCI buses for devices (used to implement device discovery)
void pciScan(pciFunction_t func, int type, void *extra) {
    if ((pciConfigReadField(0, PCI_OFFSET_HEADERTYPE, 1) & 0x80) == 0) {
        pciScanBus(func, type, 0, extra);
        return;
    }

    int hit = 0;
    for (int f = 0; f < 8; f++) {
        uint32_t dev = (uint32_t)((0 << 16) | (0 << 8) | f);
        if (pciConfigReadField(dev, PCI_OFFSET_VENDORID, 2) != PCI_NONE) {
            hit = 1;
            pciScanBus(func, type, f, extra);
        } else {
            break;
        }
    }

    if (!hit) {
        for (int bus = 0; bus < 256; bus++) {
            for (int slot = 0; slot < PCI_MAX_SLOTS; slot++) {
                pciScanSlot(func, type, bus, slot, extra);
            }
        }
    }
}


uint16_t pciGetVendorID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)pciConfigRead(bus, device, function, PCI_OFFSET_VENDORID);
}



uint16_t pciGetDeviceID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)pciConfigRead(bus, device, function, PCI_OFFSET_DEVICEID);
}

uint16_t pciGetClassID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)(((uint32_t)pciConfigRead(bus, device, function, PCI_OFFSET_CLASSID)) & ~0x00FF) >> 8;
}

uint16_t pciGetSubClassID(uint16_t bus, uint16_t device, uint16_t function) {
    return (uint16_t)(((uint32_t)pciConfigRead(bus, device, function, PCI_OFFSET_SUBCLASSID)) & ~0xFF00);
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

    // Probe PCI devices
    pciProbeForDevices();

    // Make sure the functions know initialization has completed.
    isPCIInitialized = true;
}


// Returns the PciDevTable ID of of something
int getDevTableId(uint32_t deviceID, uint32_t vendorID) {
    int id = -1;
    for (uint32_t i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (PciDevTable[i].DevId == deviceID && PciDevTable[i].VenId == vendorID) {
            id = i;
            break;
        }
    }
    
    return id;
}


// Returns the class ID type of something
int getClassIdType(uint16_t classID, uint16_t subclassID) {
    int id = -1;

    for (uint32_t i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].BaseClass == classID && PciClassCodeTable[i].SubClass == subclassID) {
            id = i;
            break;
        }
    }

    return id;
}

void printPCIInfo() {

    for (uint32_t i = 0; i < dev_idx; i++) {
        
        // Get the class type
        uint16_t classID = pciGetClassID(pciDevices[i]->bus, pciDevices[i]->slot, pciDevices[i]->func);
        uint16_t subclassID = pciGetSubClassID(pciDevices[i]->bus, pciDevices[i]->slot, pciDevices[i]->func);
        int classTypeId = getClassIdType(classID, subclassID);        

        int id = getDevTableId(pciDevices[i]->device, pciDevices[i]->vendor);
        if (id != -1 && classTypeId != -1) {
            if (!PciClassCodeTable[classTypeId].ProgDesc) {
                printf("%i) %s %s (%s - %s - %s)\n", i, PciDevTable[id].Chip, PciDevTable[id].ChipDesc, PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc, PciClassCodeTable[classTypeId].ProgDesc);
            } else {
                printf("%i) %s %s (%s - %s)\n", i, PciDevTable[id].Chip, PciDevTable[id].ChipDesc, PciClassCodeTable[classTypeId].BaseDesc, PciClassCodeTable[classTypeId].SubDesc);
            }
        } else if (classTypeId != -1) {
            // We don't know the exact device type - that's ok, just print Unknown and the class type
            if (!PciClassCodeTable[classTypeId].ProgDesc) {
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


// pciGetInterrupt(uint32_t device) - Returns the PCI interrupt for a specific device
int pciGetInterrupt(uint32_t device) {
    return pciConfigReadField(device, PCI_INTERRUPT_LINE, 1);
}
