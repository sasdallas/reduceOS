// ======================================================================
// bios32.c - Handles real-mode interrupt calls in protected mode
// ======================================================================

#include "include/bios32.h" // Main header file


idtPtr_t realModeGDT;
idtPtr_t realModeIDT;

extern gdtEntry_t gdtEntries[8];

void (*bios32_execute)() = (void *)0x7c00;

void bios32_init() {
    // Set up the 16-bit code and data segment in GDT.
    gdtSetGate(6, 0, 0xFFFFFFFF, 0x9A, 0x0F);
    gdtSetGate(7, 0, 0xFFFFFFFF, 0x92, 0x0F);

    // Setup the real mode GDT ptr
    realModeGDT.base_addr = (uint32_t)gdtEntries;
    realModeGDT.limit = sizeof(gdtEntries) - 1;

    // Setup the real mode IDT ptr
    realModeIDT.base_addr = 0;
    realModeIDT.limit = 0x3FF;
}

// bios32_call(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out) - Calls bios32.
void bios32_call(uint8_t interrupt, REGISTERS_16 *in, REGISTERS_16 *out) {
    void *newCodeBase = (void *)0x7c00;

    // Copy the GDT entries to the BIOS32 GDT entries.
    memcpy(&bios32_gdt_entries, gdtEntries, sizeof(gdtEntries));

    // Update the base address of the GDT entries, starting from 0x7C00.
    realModeGDT.base_addr = (uint32_t)REBASE_ADDRESS((&bios32_gdt_entries));

    // Copy the real mode GDT and IDT to their respective pointers.
    memcpy(&bios32_gdt_ptr, &realModeGDT, sizeof(idtPtr_t));
    memcpy(&bios32_idt_ptr, &realModeIDT, sizeof(idtPtr_t));

    // Copy the in registers to their pointers.
    memcpy(&bios32_in_reg16_ptr, in, sizeof(REGISTERS_16));
    
    // Get the in registers' address.
    void *in_reg16_address = REBASE_ADDRESS(&bios32_in_reg16_ptr);

    // Copy the BIOS interrupt number to its respective pointer.
    memcpy(&bios32_int_number_ptr, &interrupt, sizeof(uint8_t));

    // Copy the bios32 code to a new address.
    uint32_t size = (uint32_t)BIOS32_END - (uint32_t)BIOS32_START;
    memcpy(newCodeBase, BIOS32_START, size);

    // Start executing the BIOS32 code.
    bios32_execute();

    // Copy the output registers to the out ptr.
    in_reg16_address = REBASE_ADDRESS(&bios32_out_reg16_ptr);
    memcpy(out, in_reg16_address, sizeof(REGISTERS_16));

    // Reinitialize GDT and IDT
    gdtInit();
    idtInit();
}