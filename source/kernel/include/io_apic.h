// io_apic.h - header file for io_apic.c

#ifndef IO_APIC_H
#define IO_APIC_H

// Includes
#include <stdint.h> // Integer declarations

// Definitions

// Registers for the IO APIC (memory mapped)
#define IO_APIC_REGSEL 0x00
#define IO_APIC_WIN 0x10

// Registers for the IO APIC
#define IO_APIC_ID 0x00
#define IO_APIC_VER 0x01
#define IO_APIC_ARB 0x02
#define IO_APIC_REDTBL 0x10

// Functions
void ioAPIC_setEntry(uint8_t *base, uint8_t index, uint64_t data); // Set an entry in the IO APIC.
void ioAPIC_init(); // Initialize the IO APIC.

#endif
