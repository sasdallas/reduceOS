// gdt.h - header file for the global descriptor table.

#ifndef GDT_H
#define GDT_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/terminal.h" // printf
#include "include/libc/assert.h" // ASSERT() macro

// Definitions
#define MAX_DESCRIPTORS 8

// Typedefs

// gdtEntry - defines one GDT entry.
struct gdtEntry_struct {
    uint16_t limitLow; // Lower 16-bits of the limit
    uint16_t baseLow; // Lower 16 bits of the base.
    uint8_t baseMiddle; // Next 8 bits of the base
    uint8_t access; // Access flags (determine what ring the segment can be used in)
    uint8_t granularity;
    uint8_t baseHigh; // Last 8 bits of the base.
} __attribute__((packed));

typedef struct gdtEntry_struct gdtEntry_t;

// gdtPtr - defined a GDT pointer (points to start of array of GDT etnries)
struct gdtPtr_struct {
    uint16_t limit; // Upper 16 bits of all selector limits
    uint32_t base; // Address of the 1st gdtEntry_t struct.
} __attribute__((packed));

typedef struct gdtPtr_struct gdtPtr_t;

// External functions
extern void install_gdt(uint32_t);

// Functions
void gdtSetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran); // Set the value of 1 GDT entry.
void gdtInit(); // Installs the GDT ptr and sets up all the registers

#endif