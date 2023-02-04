// ==================================================
// gdt.c - Global Descriptor Table initializer
// ==================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

/* Developer note: When reduceOS switched to multiboot, I totally forgot about this file. I'm not sure if this is why paging is failing, but it would be pretty funny if so. */

#include "include/gdt.h" // Main header file

// Variable definitions
gdtEntry_t gdtEntries[MAX_DESCRIPTORS];
gdtPtr_t gdtPtr;
static tss_t taskStateSegment __attribute__ ((aligned (4096)));



// Functions:



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
    gdtPtr.limit = (sizeof(gdtEntry_t) * 5) - 1;
    gdtPtr.base = (uint32_t)&gdtEntries;

    // Now setup the GDT entries
    gdtSetGate(0, 0, 0, 0, 0); // Null segment
    gdtSetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
    gdtSetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdtSetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
    gdtSetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment.

    // Setup the TSS.
    memset(&taskStateSegment, 0x00, sizeof(tss_t));
    taskStateSegment.eflags = 0x1202;
    taskStateSegment.ss0 = 0x10; // Data segment
    taskStateSegment.esp0 = 0xDEADBEEF; // Invalid psuedo-address
    taskStateSegment.cs = 0x0B;
    taskStateSegment.ss = taskStateSegment.ds = taskStateSegment.es = taskStateSegment.fs = taskStateSegment.gs = 0x13;
    gdtSetGate(5, (uint32_t)(&taskStateSegment), sizeof(tss_t)-1, 0x80 | 0x09 | 0x00, 0x40);


    // Install the GDT.
    install_gdt((uint32_t)&gdtPtr);
    

    printf("GDT initialized\n");
}

// Now, for the kernel stack methods

// setKernelStack() - Set the kernel stack.
extern task_t currentTask;
void setKernelStack() {
    task_t *current_task = &currentTask;
    // 16384 is the kernel stack size.
    taskStateSegment.esp0 = (size_t)current_task->stackStart + 16384-16;

}