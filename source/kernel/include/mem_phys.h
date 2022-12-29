// mem_phys.h - Header file for mem_phys.c (physical memory management)
// This implementation is by BrokenThorn Entertainment, modified to support C.

#ifndef MEM_PHYS_H
#define MEM_PHYS_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions.
#include "include/libc/stddef.h" // size_t

// Definitions
#define PHYS_BLOCKS_PER_BYTE 8 // 8 blocks per byte
#define PHYS_BLOCK_SIZE 4096 // Block size (always 4k)
#define PHYS_BLOCK_ALIGN 4906 // Block align (also 4k)

// Typedefs
typedef uint32_t physicalAddress;

// Memory region structure
typedef struct {
    uint32_t startLo; // Low bits of base address
    uint32_t startHi; // High bits of base address
    uint32_t sizeLo; // Low bits of length (in bytes)
    uint32_t sizeHi; // High bits of length (in bytes)
    uint32_t type; // Type of region.
    uint32_t acpi_3_0; // ACPI 3.0?
} memoryRegion;

// Functions
void memPhys_init(size_t memSize, physicalAddress bitmap); // Initializes physical memory management and sets up values.
void memPhys_initRegion(physicalAddress base, size_t size); // Initialize a region in physical memory.
void memPhys_deinitRegion(physicalAddress base, size_t size); // Deinitialize a region in physical memory.
void *memPhys_allocateBlock(); // Allocates a block in physical memory.
void memPhys_freeBlock(void *p); // Frees a block in physical memory.
void *memPhys_allocateBlocks(size_t size); // Allocates size amount of blocks.
void memPhys_freeBlocks(void *p, size_t size); // Free size amount of blocks.
void enablePaging(); // Enable paging.
bool isPaging(); // Check if paging is enabled.
void loadPDBR(physicalAddress address); // Load PDBR
physicalAddress getPDBR(); // Get address of PDBR
#endif