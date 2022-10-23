// GDT - Global Descriptor Table
#ifndef GDT_H
#define GDT_H

#include "stdint.h"

#define NO_GDT_DESCRIPTORS     8

typedef struct {
    uint16_t segment_limit;  // segment limit first 0-15 bits
    uint16_t base_low;       // base first 0-15 bits
    uint8_t base_middle;     // base 16-23 bits
    uint8_t access;          // access byte
    uint8_t granularity;     // high 4 bits (flags) low 4 bits (limit 4 last bits)(limit is 20 bit wide)
    uint8_t base_high;       // base 24-31 bits
} __attribute__((packed)) GDT;

typedef struct {
    uint16_t limit;       // limit size of all GDT segments
    uint32_t base_address;  // base address of the first GDT segment
} __attribute__((packed)) GDT_PTR;

// asm gdt functions, define in load_gdt.asm
extern void load_gdt(uint32_t gdt_ptr);

/**
 * fill entries of GDT 
 */
void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

// initialize GDT
void gdt_init();

#endif