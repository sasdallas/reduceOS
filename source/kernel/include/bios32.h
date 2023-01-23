// bios32.h - header file for bios32.c (handles BIOS calls in protected mode)

#ifndef BIOS32_H
#define BIOS32_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/gdt.h" // Global descriptor table
#include "include/idt.h" // Interrupt descriptor table
#include "include/regs.h" // Registers

// External stuff defined in bios.asm
extern void BIOS32_START();
extern void BIOS32_END();
extern void *bios32_gdtPtr;
extern void *bios32_gdtEntries;
extern void *bios32_idtPtr;
extern void *bios32_inReg_ptr;
extern void *bios32_outReg_ptr;
extern void *bios32_intNo_ptr;


// Macros
#define BASE_ADDRESS(x) (void*)(0x7C00 + (void*)x - (uint32_t)BIOS32_START)

// Functions
void bios32_init(); // Initialize the BIOS 32 routine by setting the real mode GDT and IDT.
void bios32_call(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out); // Copy data to codeBase and execute the code in real mode.
#endif