// ======================================================================
// bios32.c - Handles real-mode interrupt calls in protected mode
// ======================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code

#include "include/bios32.h" // Main header file

gdtPtr_t realMode_gdt;
idtPtr_t realMode_idt;

extern gdtEntry_t gdtEntries[8];
void (*bios32_function)() = (void *)0x7C00;



// bios32_init() - Initialize the BIOS 32 routine by setting the real mode GDT and IDT.
void bios32_init() {
    gdtSetGate(6, 0, 0xFFFFFFFF, 0x9A, 0x0F); // Real mode code segment
    gdtSetGate(7, 0, 0xFFFFFFFF, 0x92, 0x0F); // Real mode data segment.

    // Setup the real mode GDT ptr.
    realMode_gdt.base = (uint32_t)gdtEntries;
    realMode_gdt.limit = sizeof(gdtEntries) - 1;

    // Setup the real mode IDT ptr.
    realMode_idt.base_addr = 0;
    realMode_idt.limit = 0x3FF;
}


// bios32_call(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out) - Copy data to codeBase and execute the code in real mode.
void bios32_call(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out) {
    void *codeBase = (void *)0x7C00;

    // Copy the GDT entries to bios32_gdtEntires (defined in bios.asm)
    memcpy(&bios32_gdtEntries, gdtEntries, sizeof(gdtEntries));
    
    // Set the base address of the BIOS32 GDT entries starting from 0x7C00.
    realMode_gdt.base = (uint32_t)BASE_ADDRESS((&bios32_gdtEntries));
    
    // Copy real mode GDT to GDT pointer (same with the real mode IDT)
    memcpy(&bios32_gdtPtr, &realMode_gdt, sizeof(gdtPtr_t));
    memcpy(&bios32_idtPtr, &realMode_idt, sizeof(idtPtr_t));

    // Copy all the 16-bit registers to the pointers
    memcpy(&bios32_inReg_ptr, in, sizeof(REGISTERS_16));
    
    // Get out registers address defined in bios.asm starting from 0x7C00
    void *inRegAddress = BASE_ADDRESS(&bios32_inReg_ptr);

    // Copy BIOS interrupt number to bios32_intNo_ptr
    memcpy(&bios32_intNo_ptr, &interrupt, sizeof(uint8_t));

    uint32_t size = (uint32_t)BIOS32_END - (uint32_t)BIOS32_START;
    memcpy(BIOS32_START, codeBase, size);
    
    // Execute the code.
    bios32_function();
    
    // Copy input registers to output registers.
    inRegAddress = BASE_ADDRESS(&bios32_outReg_ptr);
    memcpy(out, inRegAddress, sizeof(REGISTERS_16));

    // Reinitialize GDT and IDT.
    gdtInit();
    idtInit();
}