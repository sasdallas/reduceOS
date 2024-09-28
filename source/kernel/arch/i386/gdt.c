// ==================================================
// gdt.c - Global Descriptor Table initializer
// ==================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.


#include <kernel/gdt.h> // Main header file

// Variable definitions
gdtEntry_t gdtEntries[MAX_DESCRIPTORS];
gdtPtr_t gdtPtr;

extern void tssFlush();

// Functions

// gdtSetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) - Set the value of 1 GDT entry.
void gdtSetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Sanity check first!
    ASSERT(num < MAX_DESCRIPTORS, "gdtSetGate()", "invalid descriptor number");

    // Now set the proper values in the gdt entries.
    gdtEntries[num].baseLow = (base & 0xFFFF);
    gdtEntries[num].baseMiddle = (base >> 16) & 0xFF;
    gdtEntries[num].baseHigh = (base >> 24) & 0xFF;

    gdtEntries[num].limitLow = (limit & 0xFFFF);
    gdtEntries[num].granularity = (limit >> 16) & 0x0F;

    gdtEntries[num].granularity |= gran & 0xF0;
    gdtEntries[num].access = access;
}


// gdtInit() - Initializes GDT and sets up all the pointers
void gdtInit() {
    // Setup the gdtPtr to point to our gdtEntires
    gdtPtr.limit = sizeof(gdtEntries)-1;
    gdtPtr.base = (uint32_t)&gdtEntries;

    // Now setup the GDT entries
    gdtSetGate(0, 0, 0, 0, 0); // Null segment
    gdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment.
    tssWrite(5, 0x10, 0x0); // Task state segment

    // Install the GDT.
    install_gdt((uint32_t)&gdtPtr);
    
    // Flush TSS.
    tssFlush();
}
