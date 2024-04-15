// pci.h - header file for pci.c (handles the peripheral component interconnect bus)

#ifndef PCI_H
#define PCI_H

// Includes
#include "include/libc/stdint.h" // Integer declarations.

#include "include/hal.h" // outportl and inportl
#include "include/terminal.h" // printf
#include "include/pci_vendors.h"

// Definitions
#define CONFIG_ADDR 0xCF8 // CONFIG_ADDR specifies the configuration address required to be accessed.
#define CONFIG_DATA 0xCFC // CONFIG_DATA specifies the configuration address generate the config address and transfers data to/from the CONFIG_DATA register.

#define MAX_BUS 16 
#define MAX_SLOTS 32

#define OFFSET_VENDORID 0
#define OFFSET_STATUS 0x5
#define OFFSET_COMMAND 0x5
#define OFFSET_DEVICEID 2
#define OFFSET_CLASSID 0xA
#define OFFSET_SUBCLASSID 0xA

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



// Functions

void initPCI(); // Initializes the PCI environment (probes devices and stuff)
void printPCIInfo(); // Prints PCI info

#endif