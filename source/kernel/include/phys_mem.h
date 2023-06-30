// phys_mem.h - header file for the physical memory manager

#ifndef PHYS_MEM_H
#define PHYS_MEM_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/stdbool.h" // Boolean declaration
#include "include/libc/stdlib.h" // size_t declaration
#include "include/serial.h" // Serial port output.

// Definitions
#define PHYS_MEM_BLOCKS_PER_BYTE 8 // 8 blocks per byte
#define PHYS_MEM_BLOCK_SIZE 4096 // 4096 blocks per byte
#define PHYS_MEM_BLOCK_ALIGN PHYS_MEM_BLOCK_SIZE // Block alignment

// Typedefs
typedef struct {
    uint32_t startLo;
    uint32_t startHi;
    uint32_t sizeLo;
	uint32_t sizeHi;
	uint32_t type;
	uint32_t acpi_3_0;
} physicalMemoryRegion;

// Functions

#endif