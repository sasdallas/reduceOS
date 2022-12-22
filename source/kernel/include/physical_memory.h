// physical_memory.h - header file for physical_memory.c (manages physical memory)

#ifndef PHYISCAL_MEMORY_H
#define PHYISCAL_MEMORY_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions

#include "include/terminal.h" // Terminal functions like printf

// Typedefs
typedef uint32_t physicalAddress; // Physical address of blocks / bits.

// Memory region structure
typedef struct {
	uint32_t	startLo;
	uint32_t	startHi;
	uint32_t	sizeLo;
	uint32_t	sizeHi;
	uint32_t	type;
	uint32_t	acpi_3_0;
} memoryRegion;




// Definitions
#define BLOCKS_PER_BYTE 8        // 8 blocks per byte
#define BLOCK_SIZE 4096          // Block size (always has to be 4k)
#define BLOCK_ALIGN BLOCK_SIZE   // Block align also has to be 4k




// Functions
// These should later be moved to a libc header file - but for now they stay like this.

void physMemoryInit(size_t memSize, physicalAddress bitmap); // Initiates the physical memory management (sets all proper values and stuff like that)
void initRegion(physicalAddress base, size_t n); // Initiates a physical memory region.
void deinitRegion(physicalAddress base, size_t n); // Deinitializes a physical memory region.
void *allocateBlock(); // Allocates a physical memory block.
void freeBlock(void *block); // Frees a block in the memoryMap.
void *allocateBlocks(size_t size); // Allocate multiple blocks (# of blocks is size).
void freeBlocks(void *p, size_t size); // Free blocks (# of blocks is size).

// Getter functions (that return some of the static variables):
uint32_t getMemorySize();
uint32_t getBlockCount();
uint32_t getUsedBlockCount();
uint32_t getFreeBlockCount();
uint32_t getBlockSize();

#endif