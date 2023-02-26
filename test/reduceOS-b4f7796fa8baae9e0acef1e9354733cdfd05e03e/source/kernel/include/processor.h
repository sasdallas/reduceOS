// processor.h - Handles all CPU related functions

#ifndef PROCESSOR_H
#define PROCESSOR_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/gdt.h" // Global descriptor table
#include "include/idt.h" // Interrupt descriptor table
#include "include/isr.h" // Interrupt service routines

// Functions
void cpuInit(); // Initializes the CPU with ISR, IDT, and GDT
uint32_t getCPUFrequency(); // Returns the CPU frequency (todo: create processor.c).


#endif