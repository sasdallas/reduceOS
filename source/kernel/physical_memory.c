// ==========================================================
// physical_memory.c - Manages kernel physical memory 
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/physical_memory.h" // Main header file

// Note: While the primary use of this file is to manage physical memory, a few functions are used in virtual memory management (and possibly heap later), like paging.
// Another note: Whenever bit is referenced, it refers to frame.

static uint32_t memorySize = 0;     // Size of physical memory
static uint32_t usedBlocks = 0;     // Used blocks
static uint32_t maxBlocks = 0;      // Maximum available blocks
static uint32_t *memoryMap = 0;     // This represents the memory map bit array (in which each bit represents 1 memory block)


// Moving on to the actual functions...


// Some get functions...
size_t getMemorySize() { return memorySize; }
uint32_t getBlockCount() { return maxBlocks; }
uint32_t getUsedBlockCount() { return usedBlocks; }
uint32_t getFreeBlockCount() { return maxBlocks - usedBlocks; }
uint32_t getBlockSize() { return BLOCK_SIZE; }




// memoryMapSet(int bit) - Sets a bit (or frame) within the memoryMap.
void memoryMapSet(int bit) {
    memoryMap[bit / 32] |= (1 << (bit % 32)); // Set the bit
}

// memoryMapUnset(int bit) - Remove any bit (or frame) within the memoryMap.
void memoryMapUnset(int bit) {
    memoryMap[bit / 32] &= ~ (1 << (bit % 32)); // Remove the bit
}

// memoryMapTest(int bit) - Returns if a bit is present in the memoryMap.
bool memoryMapTest(int bit) {
    return memoryMap[bit / 32] & (1 << (bit % 32));
}

// memoryMapFindFree() - Finds the 1st free frame in the bit array and returns its index.
int memoryMapFindFree() {
    // Iterate thorugh the memory map to find the first free bit.
    for (uint32_t i = 0; i < getBlockCount() / 32; i++) {
        // Make sure the block isn't at the end of the memoryMap.
        if (memoryMap[i] != 0xFFFFFFFF) {
            for (int x = 0; x < 32; x++) {
                // Test each bit in the DWORD
                int bit = 1 << x;
                if (!(memoryMap[i] & bit)) {
                    return i*4*8+x; // Found the index! Return it. We do the 4*8 because there are 32 "bits" in one block, and then add the block index.
                }
            }
        }
    }

    return -1; // We couldn't find any index.
}


// memoryMapFirstFreeSize(size_t n) - Finds the first free size number of frames and returns its index.
int memoryMapFirstFreeSize(size_t n) {
    if (n == 0) return -1; // Make sure those pesks users aren't mucking up our input.
    if (n == 1) return memoryMapFindFree(); // If the size is one (1 frame), we can just call memoryMapFindFree (since it assumes size is 1)

    for (uint32_t i = 0; i < getBlockCount() / 32; i++) {
        // Same as before: Make sure block isn't at the end of memory map
        if (memoryMap[i] != 0xFFFFFFFF) {
            // We've verified that the block is a valid block. Now test each bit in the DWORD.
            for (int x = 0; x < 32; x++) {
                int bit = 1 << x;
                if (!(memoryMap[i] & bit)) {
                    int startingBit = i * 32;
                    startingBit += bit; // Get the free bit in the DWORD at index i

                    uint32_t free = 0; 
                    // Now loop through each bit to check if there's enough space.
                    for (uint32_t count = 0; count <= n; count++) {
                        if (!memoryMapTest(startingBit+count)) {
                            free++; // If the memoryMapTest fails then this bit is clear (free frame).
                        }

                        if (free == n) return i*4*8+x; // Free count == size needed. Return index. Remember that we do 4*8 because 32 "bits" per block.
                    }
                }
            }
        }
    }

    return -1; // No index, big sad.
}


// Moving onto the PROPER functions that the user will use...


// physMemoryInit(size_t memSize, physicalAddress bitmap) - Initiates the physical memory management (sets all proper values and stuff like that)
void physMemoryInit(size_t memSize, physicalAddress bitmap) {
    // Update variables.
    memorySize = memSize;
    memoryMap = (uint32_t*) bitmap;
    maxBlocks = (getMemorySize() * 1024) / BLOCK_SIZE;
    usedBlocks = maxBlocks;

    // Update memory map.
    memset(memoryMap, 0xF, getBlockCount() / BLOCKS_PER_BYTE); // By default, all memory is in use.
    printf("Physical memory management initialized.\n");
}


// initRegion(physicalAddress base, size_t n) - Initiates a physical memory region.
void initRegion(physicalAddress base, size_t n) {
    int align = base / BLOCK_SIZE; // Get alignment of region.
    int blocks = n / BLOCK_SIZE; // Get amount of blocks.

    for (; blocks >= 0; blocks--) {
        memoryMapUnset(align++); // Unset bit.
        usedBlocks--; // Lower used blocks.
    }

    memoryMapSet(0); // The first block is ALWAYS set - insuring allocations cannot be 0.
}


// deinitRegion(physicalAddress base, size_t n) - Deinitializes a physical memory region.
void deinitRegion(physicalAddress base, size_t n) {
    int align = base / BLOCK_SIZE; // Same as last time - get alignment and blocks and divide them by 4k.
    int blocks = base / BLOCK_SIZE;

    // Deallocate the region - do the exact opposite of last time, instead incrementing usedBlocks and setting memory map bits.
    for (; blocks >= 0; blocks--){
        memoryMapSet(align++);
        usedBlocks++;
    }

    // We don't need to do the memoryMapSet.
}


// allocateBlock() - Allocates a physical memory block.
void *allocateBlock() {
    if (getFreeBlockCount() <= 0) return 0; // No memory, so we can't do that.

    int frame = memoryMapFindFree(); // Find the first free index of a frame.
    if (frame == -1) return 0; // No memory.

    memoryMapSet(frame); // Set the bit in the memory map.

    physicalAddress addr = frame * BLOCK_SIZE; // Get the physical address.
    usedBlocks++; // Increment usedBlocks.

    return (void*)addr; // Return the address.
}



// freeBlock(void *block) - Frees a block in the memoryMap.
void freeBlock(void *block) {
    physicalAddress addr = (physicalAddress)block; // Get the physical address.
    int frame = addr / BLOCK_SIZE; // Get the frame (divide the address by 4k, or BLOCK_SIZE)


    memoryMapUnset(frame); // Free the block and decrement usedBlocks.
    usedBlocks--;
}

// allocateBlocks(size_t size) - Allocate multiple blocks (# of blocks is size).
void *allocateBlocks(size_t size) {
    if (getFreeBlockCount() <= size) return 0; // Too little space.

    int frame = memoryMapFirstFreeSize(size); // Find the first free index.
    if (frame == -1) return 0; // Too little space.

    for (uint32_t i = 0; i < size; i++) memoryMapSet(frame+i); // Allocate the blocks!

    physicalAddress addr = frame * BLOCK_SIZE; // Get the address of the block (we need to multiply frame by BLOCK_SIZE to get a valid address)
    usedBlocks += size;

    return (void*)addr;
}

// freeBlocks(void *p, size_t size) - Free blocks (# of blocks is size).
void freeBlocks(void *p, size_t size) {
    physicalAddress addr = (physicalAddress)p;
    int frame = addr / BLOCK_SIZE;

    for (uint32_t i = 0; i < size; i++) memoryMapUnset(frame+i); // Free the blocks!
    usedBlocks -= size; // Decrement usedBlocks.
}

