// pmm.h - Header file for physical memory management

#ifndef PMM_H
#define PMM_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions
#include "include/paging.h" // Paging declarations
#include "include/heap.h" // kmalloc()

// Variables
extern uint32_t *frames;
extern uint32_t nframes;

// Functions
void pmmInit(uint32_t physMemorySize); 
#endif