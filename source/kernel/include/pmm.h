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

// Typedefs
typedef uint32_t physical_address;

typedef struct {
	uint32_t	startLo;
	uint32_t	startHi;
	uint32_t	sizeLo;
	uint32_t	sizeHi;
	uint32_t	type;
	uint32_t	acpi_3_0;
} memoryRegion_t;



// Functions
void pmmInit(uint32_t physMemorySize); 
#endif