// ===================================================
// mem_phys.c - Physical memory management.
// ===================================================
// This implementation is not by me. Credits in the header file.
// I have modified portions of this implementation, so I ask if you EXACTLY copy and paste my code, you credit me as well as the original owner.

#include "include/mem_phys.h" // Main header file


// Variables, first off!

static uint32_t memorySize = 0; // Size of physical memory.
static uint32_t usedBlocks = 0; // Used blocks.
static uint32_t maxBlocks = 0; // Maximum number of available memory blocks.
static uint32_t *memoryMap = 0; // Memory map bit array (in which each bit represents a memory block)


// NOTE: Usually in these functions, when referring to bits, we also mean frames.


// For some quick getter functions (cause some of those are static variables)

size_t memPhys_getMemorySize() { return memorySize; }
uint32_t memPhys_getBlockCount() { return maxBlocks; }
uint32_t memPhys_getUsedBlockCount() { return usedBlocks; }
uint32_t memPhys_getFreeBlockCount() { return maxBlocks - usedBlocks; }
uint32_t memPhys_getBlockSize() { return PHYS_BLOCK_SIZE; }

// Moving on to the actual functions...
// Starting with the memory map functions...
 
// memoryMapSet(int bit) - Sets a bit/frame in the memory map bit array.
inline void memoryMapSet(int bit) {
    memoryMap[bit / 32] |= (1 << (bit % 32));
}

// memoryMapUnset(int bit) - Unset any bit/frame in the memory map bit array.
inline void memoryMapUnset(int bit) {
    memoryMap[bit / 32] &= ~ (1 << (bit % 32));
}

// memoryMapTest(int bit) - Test if any bit/frame is set in the memory map.
inline bool memoryMapTest(int bit) {
    return memoryMap[bit / 32] & (1 << (bit % 32));
}

// memoryMapFindFree() - Finds the first free frame in the bit array and returns the index (or -1 if it can't find any.)
int memoryMapFindFree() {
    // We begin by iterating through all the blocks in the map.
    for (uint32_t i = 0; i < memPhys_getBlockCount(); i++) {
        // Make sure we're not at the end of the map.
        if (memoryMap[i] != 0xFFFFFFFF) {
            // Cool! We're not! Now test each bit in the dword.
            for (int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(memoryMap[i] & bit)) return i*4*8+j;
            }
        }
    }

    return -1; // We couldn't find anything
}

// memoryMapFindFreeSize(size_t size) - Finds first free size number of frames and returns its index.
int memoryMapFindFreeSize(size_t size) {
    if (size == 0) return -1; // Invalid size.
    if (size == 1) return memoryMapFindFree(); // We don't need to do this, just do memoryMapFindFree (1 frame)

    // Like last time, iterate through all the blocks in the map.
    for (uint32_t i = 0; i < memPhys_getBlockCount(); i++) {
        // Make sure we're not at the end of the map.
        if (memoryMap[i] != 0xFFFFFFFF) {
            // Cool! We're not - test each bit in the dword.
            for (int j = 0; j < 32; j++) {
                int bit = 1 << j;
                if (!(memoryMap[i] & bit)) {
                    // We might have something. Let's continue.
                    int startingBit = i * 32;
                    startingBit += bit; // Get the free bit in the dword at index i.

                    uint32_t free = 0; // Now loop through each bit to see if it's enough space.
                    for (uint32_t count = 0; count <= size; count++) {
                        if (!memoryMapTest(startingBit + count)) free++; // This bit is clear (free frame)

                        if (free == size) return i*4*8+j; // Free count is the size needed. Return the index.
                    }
                }
            }
        }
    }

    return -1; // We couldn't find anything :(
}

// Physical memory management functions (non bitmap):


// memPhys_init(size_t memSize, physicalAddress bitmap) - Initializes physical memory management and sets up values.
void memPhys_init(size_t memSize, physicalAddress bitmap) {
    
    // Setup all the values
    memorySize = memSize;
    memoryMap = (uint32_t*)bitmap;
    maxBlocks = (memPhys_getMemorySize() * 1024) / PHYS_BLOCK_SIZE;
    usedBlocks = memPhys_getBlockCount();


    memset(memoryMap, 0xF, memPhys_getBlockCount() / PHYS_BLOCKS_PER_BYTE); // By default, all memory is in use.
}

// memPhys_initRegion(physicalAddress base, size_t size) - Initialize a region in physical memory.
void memPhys_initRegion(physicalAddress base, size_t size) {
    // Get the region blocks and alignment.
    int align = base / PHYS_BLOCK_SIZE;
    int blocks = size / PHYS_BLOCK_SIZE;

    // Deinitialize all the blocks.
    for (; blocks > 0; blocks--) {
        memoryMapUnset(align++);
        usedBlocks--;
    }

    memoryMapSet(0); // First block is always set (this ensures allocations cannot be zero)
}

// memPhys_deinitRegion(physicalAddress base, size_t size) - Deinitialize a region in physical memory.
void memPhys_deinitRegion(physicalAddress base, size_t size) {
    // Like last time, first get the region blocks and alignment.
    int align = base / PHYS_BLOCK_SIZE;
    int blocks = size / PHYS_BLOCK_SIZE;

    // Initialize all the blocks (this is where it's different)
    for (; blocks > 0; blocks--) {
        memoryMapSet(align++);
        usedBlocks++;
    }

    memoryMapSet(0); // First block is always set (this ensures allocations cannot be zero)
}



// memPhys_allocateBlock() - Allocates a block in physical memory.
void *memPhys_allocateBlock() {
    if (memPhys_getFreeBlockCount() <= 0) return 0; // We are out of memory.

    int frame = memoryMapFindFree(); // Get the first free frame index.
    if (frame == -1) return 0; // Out of memory.

    memoryMapSet(frame); // Set the frame.

    // Get the address and increment used blocks.
    physicalAddress addr = frame * PHYS_BLOCK_SIZE;
    usedBlocks++;

    return (void*)addr;
}

// memPhys_freeBlock(void *p) - Frees a block in physical memory.
void memPhys_freeBlock(void *p) {
    // Get the physical address and frame of block p (frame is not aligned to 4k).
    physicalAddress addr = (physicalAddress)p;
    int frame = addr / PHYS_BLOCK_SIZE;

    memoryMapUnset(frame); // Unset the value.
    usedBlocks--;
}


// memPhys_allocateBlocks(size_t size) - Allocates size amount of blocks.
void *memPhys_allocateBlocks(size_t size) {
    if (memPhys_getFreeBlockCount() <= size) return 0; // Not enough space for allocation
    
    // Find first free index of "size" frames.
    int frame = memoryMapFindFreeSize(size);
    if (frame == -1) return 0; // Not enough space.

    // Set the values in the memory map.
    for (uint32_t i = 0; i < size; i++) memoryMapSet(frame+i);

    physicalAddress addr = frame * PHYS_BLOCK_SIZE;
    usedBlocks += size;

    return (void*)addr;
}

// memPhys_freeBlocks(void *p, size_t size) - Free size amount of blocks.
void memPhys_freeBlocks(void *p, size_t size) {
    // First, get the physical address and frame.
    physicalAddress addr = (physicalAddress)p;
    int frame = addr / PHYS_BLOCK_SIZE;

    // Do the freeing.
    for (uint32_t i = 0; i < size; i++) memoryMapUnset(frame+i);
    usedBlocks -= size;
}

// Paging functions:

void enablePaging() {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}

bool isPaging() {
    uint32_t res = 0;

    asm volatile ("mov %%eax, %%cr0" ::: "eax");
    asm volatile ("" : "=a"(res));

    return (res & 0x80000000) ? false : true;
}

void loadPDBR(physicalAddress address) {
    asm volatile ("mov %%eax, %0" :: "r"(address) : "eax");
    asm volatile ("mov %%cr3, %%eax" ::: "eax");
}

physicalAddress getPDBR() {
    asm volatile ("mov %%eax, %%cr3" ::: "eax");
    physicalAddress ret;
    asm volatile ("" : "=a"(ret));
    return ret;
}