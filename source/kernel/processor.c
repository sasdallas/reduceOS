// ======================================================
// processor.c - Handles all CPU related functions
// ======================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/processor.h" // Main header file


// Variables
static uint32_t cpuFrequency = 0;



// Functions:


// cpuInit() - Initializes the CPU with ISR, IDT, and GDT
void cpuInit() {
    // Load GDT and IDT (IDT method sets up ISR as well as PIC)
    gdtInit();
    idtInit();

    serialPrintf("GDT, IDT, and ISR have initialized successfully.\n");

    // Enable interrupts.
    enableHardwareInterrupts();
    serialPrintf("sti instruction did not fault - interrupts enabled.\n");
}

// detectCPUFrequency() - Detect the CPU frequency.
uint32_t detectCPUFrequency() {
	uint64_t start, end, diff;
	uint64_t ticks, old;

	if (cpuFrequency > 0)
		return cpuFrequency;

	old = i86_pitGetTickCount();

	// Wait for the next time slice.
	while((ticks = i86_pitGetTickCount()) - old == 0) continue;

    asm volatile ("rdtsc" : "=A"(start));
	// Wait a second to determine frequency.
	while(i86_pitGetTickCount() - ticks < 1000) continue;
	asm volatile ("rdtsc" : "=A"(end));

	diff = end > start ? end - start : start - end;
	cpuFrequency = (uint32_t) (diff / (uint64_t) 1000000);

	return cpuFrequency;
}


// getCPUFrequency() - Returns the CPU frequency.
uint32_t getCPUFrequency() {	
	if (cpuFrequency > 0)
		return cpuFrequency;

	return detectCPUFrequency();
}

