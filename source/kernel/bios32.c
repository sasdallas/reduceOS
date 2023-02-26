// ======================================================================
// bios32.c - Handles real-mode interrupt calls in protected mode
// ======================================================================


// NOTICE TO ALL WHO READ THIS FILE:
// I DID NOT WRITE THIS FILE, IT WAS WRITTEN BY pritamzope
// I AM PLANNING ON REWRITING THIS FILE TO BE FOR REDUCEOS
// NONE OF THE CODE IN bios32.c, bios32.h, bios.asm IS MINE!



#include "include/bios32.h" // Main header file


idtPtr_t g_real_mode_gdt;
idtPtr_t g_real_mode_idt;

extern gdtEntry_t gdtEntries[8];

void (*exec_bios32_function)() = (void *)0x7c00;

void bios32_init() {
    // Setup the 16-bit code and data segment in GDT.
    gdtSetGate(6, 0, 0xFFFFFFFF, 0x9A, 0x0F);
    gdtSetGate(7, 0, 0xFFFFFFFF, 0x92, 0x0F);

    // Setup the real mode GDT ptr
    g_real_mode_gdt.base_addr = (uint32_t)gdtEntries;
    g_real_mode_gdt.limit = sizeof(gdtEntries) - 1;

    // Setup the real mode IDT ptr
    g_real_mode_idt.base_addr = 0;
    g_real_mode_idt.limit = 0x3FF;
}

/**
 copy data to assembly bios32_call.asm and execute code from 0x7c00 address
*/
void bios32_service(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out) {
    void *new_code_base = (void *)0x7c00;

    // copy our GDT entries gdtEntries to bios32_gdt_entries(bios32_call.asm)
    memcpy(&bios32_gdt_entries, gdtEntries, sizeof(gdtEntries));
    // set base_address of bios32_gdt_entries(bios32_call.asm) starting from 0x7c00
    g_real_mode_gdt.base_addr = (uint32_t)REBASE_ADDRESS((&bios32_gdt_entries));
    // copy g_real_mode_gdt to bios32_gdt_ptr(bios32_call.asm)
    memcpy(&bios32_gdt_ptr, &g_real_mode_gdt, sizeof(idtPtr_t));
    // copy g_real_mode_idt to bios32_idt_ptr(bios32_call.asm)
    memcpy(&bios32_idt_ptr, &g_real_mode_idt, sizeof(idtPtr_t));
    // copy all 16 bit in registers to bios32_in_reg16_ptr(bios32_call.asm)
    memcpy(&bios32_in_reg16_ptr, in, sizeof(REGISTERS_16));
    // get out registers address defined in bios32_call.asm starting from 0x7c00
    void *in_reg16_address = REBASE_ADDRESS(&bios32_in_reg16_ptr);
    // copy bios interrupt number to bios32_int_number_ptr(bios32_call.asm)
    memcpy(&bios32_int_number_ptr, &interrupt, sizeof(uint8_t));

    // copy bios32_call.asm code to new_code_base address
    uint32_t size = (uint32_t)BIOS32_END - (uint32_t)BIOS32_START;
    memcpy(new_code_base, BIOS32_START, size);
    // execute the code from 0x7c00
    exec_bios32_function();
    // copy output registers to out
    in_reg16_address = REBASE_ADDRESS(&bios32_out_reg16_ptr);
    memcpy(out, in_reg16_address, sizeof(REGISTERS_16));

    // re-initialize the gdt and idt
    gdtInit();
    idtInit();
}