// stdlib.h - replacement for standard C header file. Contains (mostly) memory allocation functions.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STDLIB_H
#define STDLIB_H

// Includes
#include "include/libc/stdint.h" // Integer declarations, like uint8_t.
#include "include/libc/string.h" // String functions
#include "include/libc/assert.h" // ASSERT macro

#include "include/heap.h" // Functions for if kheap is not 0.

// Definitions (variables)
extern uint32_t placement_address; // Defined in heap.c

// Functions

/* To avoid circular dependencies (of ordered_array and heap), reduceOS uses stdlib.c to house kmalloc_int and a few other functions. If kernelHeap is not 0, however, it hands control back to heap.c. */

// Some functions use this key (kmalloc_ functions):
// a - align placement address
// p - return physical address
// ap - do both
// none - do neither
// They do not have function descriptions, since those are not required.

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys); // Allocates size blocks on placement_address (a uint32_t pointer to end)
uint32_t kmalloc_a(uint32_t size);
uint32_t kmalloc_p(uint32_t size, uint32_t *phys);
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);
uint32_t kmalloc(uint32_t size); 





#endif