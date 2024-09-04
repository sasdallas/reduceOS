// pci.h - header file for pci.c (handles the peripheral component interconnect bus)

#ifndef PCI_H
#define PCI_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations.

#include <kernel/hal.h> // outportl and inportl
#include <kernel/terminal.h> // printf
#include <kernel/pci_vendors.h>

// Definitions
#define PCI_CONFIG_ADDR 0xCF8 // PCI_CONFIG_ADDR specifies the configuration address required to be accessed.
#define PCI_CONFIG_DATA 0xCFC // PCI_CONFIG_DATA specifies the configuration address generate the config address and transfers data to/from the CONFIG_DATA register.

#define PCI_MAX_BUS 16 
#define PCI_MAX_SLOTS 32

#define PCI_OFFSET_VENDORID         0x00
#define PCI_OFFSET_DEVICEID         0x02
#define PCI_OFFSET_COMMAND          0x04
#define PCI_OFFSET_STATUS           0x06
#define PCI_OFFSET_REVISION         0x08

#define PCI_OFFSET_PROGIF           0x09
#define PCI_OFFSET_SUBCLASSID       0x0A
#define PCI_OFFSET_CLASSID          0x0B
#define PCI_OFFSET_CACHELINESIZE    0x0C
#define PCI_OFFSET_LATENCY          0x0D
#define PCI_OFFSET_HEADERTYPE       0x0E
#define PCI_OFFSET_BIST             0x0F
#define PCI_OFFSET_BAR0             0x10
#define PCI_OFFSET_BAR1             0x14
#define PCI_OFFSET_BAR2             0x18
#define PCI_OFFSET_BAR3             0x1C
#define PCI_OFFSET_BAR4             0x20
#define PCI_OFFSET_BAR5             0x24

#define PCI_INTERRUPT_LINE          0x3C
#define PCI_INTERRUPT_PIN           0x3D

#define PCI_SECONDARY_BUS           0x19

#define PCI_HEADERTYPE_DEVICE       0
#define PCI_HEADERTYPE_BRIDGE       1
#define PCI_HEADERTYPE_CARDBUS      2

#define PCI_TYPE_BRIDGE             0x0604
#define PCI_TYPE_SATA               0x0106

#define PCI_NONE                    0xFFFF



// Typedefs

struct __pciDriver; // Hack to make pciDevice work

typedef struct {
    uint32_t bus;
    uint32_t slot;
    uint32_t vendor;
    uint32_t device;
    uint32_t func;
    struct __pciDriver *driver;
} pciDevice;

typedef struct {
    uint32_t vendor;
    uint32_t device;
    uint32_t func;
} pciDeviceID;

typedef struct __pciDriver {
    pciDeviceID devId;
    char *deviceName;
    uint8_t (*initDevice)(pciDevice*);
    uint8_t (*initDriver)(void);
    uint8_t (*stopDriver)(void);    
} pciDriver;


typedef void (*pciFunction_t)(uint32_t device, uint16_t vendor_id, uint16_t device_id, void * extra);

// Macros
#define PCI_BUS(device) (uint8_t)((device >> 16))
#define PCI_SLOT(device) (uint8_t)((device >> 8))
#define PCI_FUNC(device) (uint8_t)(device)
#define PCI_ADDR(device, field) (0x80000000 | (PCI_BUS(device) << 16) | (PCI_SLOT(device) << 11) | (PCI_FUNC(device) << 8) | ((field) & 0xFC))



// Functions

void initPCI(); // Initializes the PCI environment (probes devices and stuff)
void printPCIInfo(); // Prints PCI info
uint32_t pciConfigReadField(uint32_t device, int field, int size); // A simplified version of pciConfigRead() that will take 3 parameters and read from a PCI device config space field.
void pciConfigWriteField(uint32_t device, int field, int size, uint32_t value); // Similar to pciConfigReadField, this will write instead.
uint16_t pciGetType(uint32_t dev); // Returns the PCI type of a device
void pciScan(pciFunction_t func, int type, void *extra); // Scans the PCI buses for devices (used to implement device discovery)
int pciGetInterrupt(uint32_t device); // Returns the interrupt of a specific device
void pciScanBus(pciFunction_t func, int type, int bus, void *extra); // Scan each slot on the bus
void pciScanSlot(pciFunction_t func, int type, int bus, int slot, void *extra); // Scan specific slot
void pciScanFunc(pciFunction_t f, int type, int bus, int slot, int func, void *extra); // Scanning the function of a PCI device
#endif
