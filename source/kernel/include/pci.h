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

// Typedefs
typedef struct {
    uint8_t slot; // PCI slot
    uint8_t bus; // PCI bus
    uint8_t irq; // PCI IRQ
    uint32_t base[6]; // The base of the PCI device
    uint32_t size[6]; // The size of the PCI device.
    uint8_t type[6]; // Type of the PCI device
} pci_info;


// Functions

void initPCI(); // Initializes the PCI environment (probes devices and stuff)
void printPCIInfo(); // Prints PCI info

#endif