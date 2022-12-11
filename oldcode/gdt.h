// gdt.h - header file for gdt.c
// Sets up & handles the Global Descriptor Table (GDT) 

// gdt.h is slightly similar to idt.h in the structures and misc. stuff


#ifndef GDT_H
#define GDT_H


// Includes
#include "include/libc/stdint.h"
#include "include/terminal.h"

// Definitions
#define MAX_DESCRIPTORS 8 // The maximum amount of descriptors allowed

// GDT Descriptor access bit flags
#define I86_GDT_DESC_ADDRESS        0x0001 // Set access bit - 00000001
#define I86_GDT_DESC_READWRITE      0x0002 // Descriptor is RW. Default is read only. - 00000010
#define I86_GDT_DESC_EXPANSION      0x0004 // Set expansion director bit - 00000100
#define I86_GDT_DESC_EXEC_CODE      0x0008 // Executable code segment - 00001000
#define I86_GDT_DESC_CODEDATA       0x0010 // Set code or data descriptor - 00010000
#define I86_GDT_DESC_DPL            0x0060 // Set DPL bits - 01100000
#define I86_GDT_DESC_MEMORY         0x0080 // Set in memory bit

// GDT descriptor granularity bit flags

#define I86_GDT_GRAND_LIMITHI_MASK      0x0f // Masks out limitHi (High 4 bits of limit) - 00001111
#define I86_GDT_GRAND_OS                0x10 // Set OS defined bit - 00010000
#define I86_GDT_GRAND_32BIT             0x40 // Set if 32-bit, default is 16-bit - 01000000
#define I86_GDT_GRAND_4K                0x80 // 4k granularity. Default: none - 10000000


// Typedefs


// GDT descriptor - defines the properties of a specific memory block and permissions
typedef struct {
    uint16_t limit; // Bits 0-15 of the segment limit
    uint16_t baseLow; // Bits 0-15 of the base address
    uint8_t baseMid; // Bits 16-23 of the base address
    uint16_t baseHigh; // Bits 24-32 of the base address
    uint8_t flags; // Descriptor access flags
    uint8_t grand;
} gdt_descriptor;

// GDTR - This will go into the processor's GDTR register.
typedef struct {
    uint16_t m_limit; // Size of GDT
    uint32_t m_base; // Base address of GDT.
} gdtr;


// Functions

extern void gdtSetDescriptor(uint32_t i, uint32_t base, uint32_t limit, uint8_t access, uint8_t grand); // Setup a descriptor in the GDT
extern gdt_descriptor* i86_gdtGetDescriptor(int i); // Returns a descriptor.
extern int gdtInitialize(); // Initialize the GDT


#endif