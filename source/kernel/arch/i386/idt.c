// =====================================================================
// idt.c - Interrupt Descriptor Table
// This file handles setting up the Interrupt Descriptor Table (IDT)
// =====================================================================

#include <kernel/idt.h> // IDT header file

extern void install_idt(uint32_t);




idtEntry_t idtEntries[I86_MAX_INTERRUPTS];
idtPtr_t idtPtr;






int idtInstallIR(uint8_t i, uint8_t flags, uint16_t segmentSelector, uint32_t base) {
    if (i < 0 || i >= I86_MAX_INTERRUPTS) return -1; // This would throw an out of bounds exception if it was.

    idtEntries[i].baseLow = base & 0xFFFF;
    idtEntries[i].baseHigh = (base >> 16) & 0xFFFF;

    idtEntries[i].segmentSelector = segmentSelector;
    idtEntries[i].reserved = 0;

    // NOTE: When user mode is enabled, make the below flags | 0x60.
    idtEntries[i].flags = flags;

    return 0;
}



void idtInit() {
    // Setup the IDT pointer.
    idtPtr.limit = sizeof(idtEntries)-1;
    idtPtr.base_addr = (uint32_t)idtEntries;

    // Clear IDT entries table.
    memset(&idtEntries, 0, sizeof(idtEntries));

    uint8_t a1, a2;
    a1 = inportb(PIC1_REG_DATA);
    a2 = inportb(PIC2_REG_DATA);

    outportb(PIC1_REG_COMMAND, 0x11);
    outportb(PIC2_REG_COMMAND, 0x11);

    outportb(PIC1_REG_DATA, 0x20);
    outportb(PIC2_REG_DATA, 0x28);

    outportb(PIC1_REG_DATA, 4);
    outportb(PIC2_REG_DATA, 2);

    outportb(PIC1_REG_DATA, 0x01);
    outportb(PIC2_REG_DATA, 0x01);
    outportb(PIC1_REG_DATA, a1);
    outportb(PIC2_REG_DATA, a2);

    isrInstall(); // Install handlers

    install_idt((uint32_t)&idtPtr);

    return;
}
