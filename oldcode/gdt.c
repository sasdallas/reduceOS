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

// gdtSetDescriptor(uint32_t i, uint32_t base, uint32_t limit, uint8_t access, uint8_t grand) - Sets up a descriptor in the GDT
void gdtSetDescriptor(uint32_t i, uint32_t base, uint32_t limit, uint8_t access, uint8_t grand) {
    //if (i > MAX_DESCRIPTORS) return; // Can't have more than 3 descriptors.
    // memset((void*)&_gdt[i], 0, sizeof(gdt_descriptor)); // We need to null out the descriptor to set proper values.

    gdt_descriptor *this = &_gdt[i];

    this->baseLow = base & 0xFFFF; // Setting up baseLow, baseMid, baseHigh, and limit (all the addresses).
    this->baseMid = (base >> 16) & 0xFF;
    this->baseHigh = (base >> 24 & 0xFF);
    this->limit = limit & 0xFFFF;

    this->flags = access; // Setting up access flags and the grandularity.
    this->grand = (limit >> 16) & 0x0F;
    this->grand = this->grand | (grand & 0xF0);

}


// i86_gdtGetDescriptor(int i) - returns a descriptor from GDT
gdt_descriptor* i86_gdtGetDescriptor(int i) {
    if (i > MAX_DESCRIPTORS) return 0;

    return &_gdt[i];
}

// gdtInitialize() - Initalize GDT
int gdtInitialize() {
    // First, we need to setup _gdtr
    _gdtr.m_limit = sizeof(_gdt) - 1;
    _gdtr.m_base = (uint32_t) _gdt;

    // Next, we set null descriptor.
    gdtSetDescriptor(0, 0, 0, 0, 0);

    // Now, we set the default code descriptor.
    gdtSetDescriptor(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Set the data segment
    gdtSetDescriptor(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdtSetDescriptor(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdtSetDescriptor(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    // Finally, install _gdtr into the CPU's gdtr register.
    installGDT();

    printf("GDT initialized.\n");
    return 0;
}

