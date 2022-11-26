// =====================================================================
// gdt.c - Global Descriptor Table
// This file handles setting up the Global Descriptor Table (GDT)
// =====================================================================

#include "include/gdt.h" // The GDT header file


// The global descriptor table is an array of descriptors.
gdt_descriptor _gdt [MAX_DESCRIPTORS];

// GDTR data (this will be passed to the ASM lgdt call)
gdtr _gdtr;




static void installGDT() {
    __asm__ ("lgdt %0" :: "m"(_gdtr));
}

// gdtSetDescriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand) - Sets up a descriptor in the GDT
void gdtSetDescriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand) {
    if (i > MAX_DESCRIPTORS) return; // Can't have more than 3 descriptors.

    memset((void*)&_gdt[i], 0, sizeof(gdt_descriptor)); // We need to null out the descriptor to set proper values.

    _gdt[i].baseLow = (uint16_t) (base & 0xffff); // Setting up baseLow, baseMid, baseHigh, and limit (all the addresses).
    _gdt[i].baseMid = (uint8_t) ((base >> 16) & 0xff);
    _gdt[i].baseHigh = (uint8_t) ((base >> 24) & 0xff);
    _gdt[i].limit = (uint16_t) (limit & 0xffff);

    _gdt[i].flags = access; // Setting up all of the flags.
    _gdt[i].grand = (uint8_t) ((limit >> 16) & 0x0f);
    _gdt[i].grand |= grand & 0xf0;

}


// i86_gdtGetDescriptor(int i) - returns a descriptor from GDT
gdt_descriptor* i86_gdtGetDescriptor(int i) {
    if (i > MAX_DESCRIPTORS) return 0;

    return &_gdt[i];
}

// gdtInitialize() - Initalize GDT
int gdtInitialize() {
    // First, we need to setup _gdtr
    _gdtr.m_limit = (sizeof (gdt_descriptor) * MAX_DESCRIPTORS) - 1;
    _gdtr.m_base = (uint32_t) &_gdt[0];

    // Next, we set null descriptor.
    gdtSetDescriptor(0, 0, 0, 0, 0);

    // Now, we set the default code descriptor.
    gdtSetDescriptor(1, 0, 0xffffffff,
        I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
        I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

    
    // Finally, install _gdtr into the CPU's gdtr register.
    installGDT();

    printf("GDT initialized.\n");
    return 0;
}

