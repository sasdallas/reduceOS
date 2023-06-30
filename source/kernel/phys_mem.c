// ====================================================
// phys_mem.c - Handles physical memory in reduceOS
// ====================================================
// Some code is sourced from BrokenThorn Entertainment's OS Development series. Please credit them if you use this code.

// Broken as of 6/15/2023 - not actively being used in kernel.c

#include "include/phys_mem.h" // Main header file

// Variables
static uint32_t physMemMemorySize = 0; // Size of physical memory
static uint32_t physMemUsedBlocks = 0; // Used blocks in physical memory
static uint32_t physMemMaxBlocks = 0; // Maximum available blocks in memory
static uint32_t* physMemMemoryMap = 0; // Physical memory map

// Static functions

// (static) physMemMemoryMapSet(int bit) - Sets a bit in the memory map
static void physMemMemoryMapSet(int bit) {
    physMemMemoryMap[bit / 32] |= (1 << (bit % 32));
}

// (static) physMemMemoryMapUnset(int bit) - Unsets a bit in the memory map
static void physMemMemoryMapUnset(int bit) {
    physMemMemoryMap[bit / 32] &= ~ (1 << (bit % 32));
}

// (static) physMemTestBit(int bit) - Tests to see if a bit is set.
static bool physMemTestBit(int bit) {
    return physMemMemoryMap[bit / 32] & (1 << (bit % 32));
}

// (static) physMemFindFirstFreeBit() - Returns index of first free bit in memory map.
static int physMemFindFirstFreeBit() {
    // Iterate through memory map to find the first free bit.
    for (uint32_t i = 0; i < physMemMaxBlocks / 32; i++) {
        // Stop if we are at maximum memory.
        if (physMemMemoryMap[i] != 0xFFFFFFFF) {
            // Check each bit in the dword (32 bits long)
            for (int j = 0; j < 32; j++) {
                // Check the bit.
                int bit = 1 << j;
                if (!(physMemMemoryMap[i] & bit)) {
                    return i*4*8+j; // Index of bit.
                }
            }
        }
    }

    return -1; // We didn't find anything :(
}

// (static) physMemFindFirstFreeBitSize(size_t size) - Returns index of first free bit with enough space in memory map.
static int physMemFindFirstFreeBitSize(size_t size) {
    if (size == 0) return -1; // Stupid callers
    if (size == 1) return physMemFindFirstFreeBit(); // bruh just use the physMemFindFirstFreeBit() method (it's a mouthful lol)

    // Iterate through memory map to find the first free bit.
    for (uint32_t i = 0; i < physMemMaxBlocks / 32; i++) {
        // Stop if we are at maximum memory.
        if (physMemMemoryMap[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                // Check the bit.
                int bit = 1 << j;
                if (!(physMemMemoryMap[i] & bit)) {
                    int startingBit = i * 32;
                    startingBit += bit;

                    uint32_t free = 0;
                    for (uint32_t count = 0; count <= size; count++) {
                        if (!physMemTestBit(startingBit+count)) free++;
                        if (free == size) return i*4*8+j;
                    }
                }
            }
        }
    }

    return -1; // We didn't find anything :(
}

// Getter functions

uint32_t physMemGetMemSize() { return physMemMemorySize; }
uint32_t physMemGetBlockCount() { return physMemMaxBlocks; }
uint32_t physMemGetUsedBlockCount() { return physMemUsedBlocks; }
uint32_t *physMemGetMemoryMap() { return physMemMemoryMap; }
uint32_t physMemGetFreeBlockCount() { return physMemMaxBlocks - physMemUsedBlocks; }

// Public functions

// physMemInit(size_t memorySize, uint32_t bitmap, uint32_t kernelSize) - Initializes the physical memory manager
void physMemInit(size_t memorySize, uint32_t bitmap, uint32_t kernelSize) {
    // Set all the variables
    physMemMemorySize = memorySize;
    physMemMemoryMap = (uint32_t*)bitmap;
    physMemMaxBlocks = (physMemGetMemSize() * 1024) / PHYS_MEM_BLOCK_SIZE;
    physMemUsedBlocks = physMemGetBlockCount();

    // On startup, all of memory is used, so fill memory map with 0xF
    memset(physMemMemoryMap, 0xF, physMemGetBlockCount() / PHYS_MEM_BLOCKS_PER_BYTE);

    // Print basic debug information.
    serialPrintf("physMemInit: initialized successfully, memory map is all set to 0xF\n");
    serialPrintf("\tphysMemMemorySize = %i KB\n\tphysMemMemoryMap = 0x%x\n\tphysMemMaxBlocks = 0x%x\n\tphysMemUsedBlocks = 0x%x\n\tphysMemFreeBlocks = 0x%x\n", physMemMemorySize, physMemMemoryMap, physMemMaxBlocks, physMemUsedBlocks, (physMemMaxBlocks-physMemUsedBlocks));

    serialPrintf("Physical memory map:\n");
    
    


    physMemDeinitRegion(0x100000, kernelSize*512);
    serialPrintf("physMemInit: Deinitialized kernel region.\n");
    serialPrintf("physMemInit: regions initialized = %i allocation blocks; used/reserved blocks = %i; free blocks = %i\n", physMemGetBlockCount(), physMemGetUsedBlockCount(), physMemGetFreeBlockCount());

}


// physMemInitRegion(uint32_t base, size_t size) - Initializes a region in physical memory.
void physMemInitRegion(uint32_t base, size_t size) {
    int align = base / PHYS_MEM_BLOCK_SIZE;
    int blocks = size / PHYS_MEM_BLOCK_SIZE;
    for (blocks>0; blocks--;) {
        physMemMemoryMapUnset(align++);
        physMemUsedBlocks--;
    }

    // First block is always set, ensuring allocations cannot be zero
    physMemMemoryMapSet(0);
}

// physMemDeinitRegion(uint32_t base, size_t size) - Deinitializes a region in physical memory
void physMemDeinitRegion(uint32_t base, size_t size) {
    int align = base / PHYS_MEM_BLOCK_SIZE;
    int blocks = size / PHYS_MEM_BLOCK_SIZE;

    for (blocks > 0; blocks--;) {
        physMemMemoryMapSet(align++);
        physMemUsedBlocks++;
    }
}

// physMemAllocateBlock() - Allocates a block in physical memory
void *physMemAllocateBlock() {
    if (physMemGetFreeBlockCount() <= 0) {
        return 0;
    }

    int frame = physMemFindFirstFreeBit();
    if (frame == -1) return 0; // We're out of memory.
    
    physMemMemoryMapSet(frame);

    uint32_t addr = frame * PHYS_MEM_BLOCK_SIZE;
    physMemUsedBlocks++;

    return (void*)addr;
}


// physMemAllocateBlocks(size_t size) - Allocates multiple blocks in physical memory
void *physMemAllocateBlocks(size_t size) {
    if (physMemGetFreeBlockCount() <= size) {
        return 0;
    }

    int frame = physMemFindFirstFreeBitSize(size);
    if (frame == -1) return 0; // We're out of memory.
    
    for (uint32_t i = 0; i < size; i++) {
        physMemMemoryMapSet(frame+i);
    }

    uint32_t addr = frame * PHYS_MEM_BLOCK_SIZE;
    physMemUsedBlocks += size;

    return (void*)addr;
}

// physMemFreeBlock(void *ptr) - Frees a block in physical memory
void physMemFreeBlock(void *ptr) {
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / PHYS_MEM_BLOCK_SIZE;
    physMemMemoryMapUnset(frame);
    physMemUsedBlocks--;   
}

// physMemFreeBlocks(void *ptr, size_t size) - Frees multiple blocks in physical memory
void physMemFreeBlocks(void *ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / PHYS_MEM_BLOCK_SIZE;

    for (uint32_t i = 0; i < size; i++) {
        physMemMemoryMapUnset(frame+i);
    }

    physMemUsedBlocks -= size;
}