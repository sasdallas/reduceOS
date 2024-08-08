// bios32.h - header file for bios32.c (handles BIOS calls in protected mode)

#ifndef BIOS32_H
#define BIOS32_H

// Includes
#include <stdint.h> // Integer definitions
#include <kernel/gdt.h> // Global descriptor table
#include <kernel/idt.h> // Interrupt descriptor table
#include <kernel/regs.h> // Registers

// External functions
extern void BIOS32_START();
extern void BIOS32_END();
extern void *bios32_gdt_ptr;
extern void *bios32_gdt_entries;
extern void *bios32_idt_ptr;
extern void *bios32_in_reg16_ptr;
extern void *bios32_out_reg16_ptr;
extern void *bios32_int_number_ptr;

#define REBASE_ADDRESS(x)  (void*)(0x7c00 + (void*)x - (uint32_t)BIOS32_START)


void bios32_init(); // Initializes BIOS32.
void bios32_call(uint8_t int_num, REGISTERS_16 *in_reg, REGISTERS_16 *out_reg); // Call the BIOS in protected mode.

#endif
