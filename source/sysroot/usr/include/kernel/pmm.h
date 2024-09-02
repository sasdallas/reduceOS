// pmm.h - Header file for physical memory management

#ifndef PMM_H
#define PMM_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/string.h> // String functions
#include <kernel/bootinfo.h> // Multiboot info
#include <kernel/serial.h>

// Variables
extern uint32_t *frames;
extern uint32_t nframes;

// Typedefs
typedef uint32_t physical_address;


// Structure for a memory region in a memory map.
// Might be GRUB-specific, dunno.


typedef enum {
    AVAILABLE = 1,
	RESERVED,
    ACPI_RECLAIMABLE,
    ACPI_NVS,
 } MULTIBOOT_MEMORY_TYPE;


typedef struct {
	uint32_t	size;
	uint32_t	addressLo;
	uint32_t	addressHi;
	uint32_t    lengthLo;
	uint32_t    lengthHi;
	MULTIBOOT_MEMORY_TYPE type;

} memoryRegion_t;



// Functions
void pmm_printMemoryMap(multiboot_info *info); // Print the memory map
void pmm_initializeMemoryMap(multiboot_info *info); // Initialize the memory map
void pmmInit(uint32_t physMemorySize); // Initialize PMM
void pmm_initRegion(uintptr_t base, size_t size); // Initialize a region as available memory
void pmm_deinitRegion(uintptr_t base, size_t size); // Deinitialize a region as unusable memory.
void *pmm_allocateBlock(); // Allocates a block
void pmm_freeBlock(void *block); // Frees a block
void *pmm_allocateBlocks(size_t size); // Allocates size amount of blocks
void pmm_freeBlocks(void *ptr, size_t size); // Frees blocks in memory
uint32_t pmm_getPhysicalMemorySize(); // Returns amount of physical memory
uint32_t pmm_getMaxBlocks(); // Returns the total amount of memory blocks
uint32_t pmm_getUsedBlocks(); // Returns the amount of used blocks
uint32_t pmm_getFreeBlocks(); // Returns the amount of free blocks

#endif
