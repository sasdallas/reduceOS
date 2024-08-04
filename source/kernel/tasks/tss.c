// =================================================================
// tss.c - Handles managing the Task State Segment
// =================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/tss.h" // Task State Segment

tss_entry_t tss;

// tssWrite(int32_t index, uint16_t ss0, uint32_t esp0) - Write to task state segment (and initalize structure)
void tssWrite(int32_t index, uint16_t ss0, uint32_t esp0) {
    // Begin by getting base and limit of entry into GDT.
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = base + sizeof(tss);

    gdtSetGate(index, base, limit, 0xE9, 0x00);

    // Ensure descriptor is 0.
    memset(&tss, 0, sizeof(tss));

    // Set the kernel stack segment and pointer.
    tss.ss0 = ss0;
    tss.esp0 = esp0;
    

    // Setup CS, SS, DS, ES, FS, and GS entries in the TSS - they specify what segments should be loaded when processor switches to kernel mode.
    tss.cs = 0x0B;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;
}



// setKernelStack(uint32_t stack) - Set the kernel stack.
void setKernelStack(uint32_t stack) {
    tss.esp0 = stack;
}
