// ==========================================================
// pmm.c - Handles physical bitmaps for memory management
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// Some code was sourced from JamesM's Kernel Development Tutorials. Please credit him as well.

#include "include/pmm.h" // Main header file

// (from JamesM's kernel dev tutorials)
#define INDEX_BIT(a) (a/(8*4))
#define OFFSET_BIT(a) (a%(8*4))

// Some bitsets of frames - used or free.
uint32_t* frames;
uint32_t nframes;

uint32_t *mmap = 0;

// Useful information for OOM errors.
uint32_t pmm_memorySize; // Physical memory size
uint32_t pmm_usedBlocks; // Used blocks of physical memory
uint32_t pmm_maxBlocks; // Total amount of blocks of physical memory


// pmmInit(uint32_t physMemorySize) - Initializes frames and nframes
void pmmInit(uint32_t physMemorySize) {
    nframes = physMemorySize / PAGE_SIZE; // Calculate number of frames
    frames = (uint32_t*)kmalloc(INDEX_BIT(nframes)); // Allocate frames array & clear it
    memset(frames, 0, INDEX_BIT(nframes));

    pmm_memorySize = physMemorySize;
    pmm_maxBlocks = (pmm_memorySize * 1024) / 4096;
    pmm_usedBlocks = pmm_maxBlocks;
}


// setFrame(uint32_t addr) - Setting a bit in the frames bitset.
void setFrame(uint32_t addr) {
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    frames[index] |= (0x1 << offset);
}


// clearFrame(uint32_t addr) - Clear a bit in the frames bitset
void clearFrame(uint32_t addr) {
    // Same as setFrame, but with a few differences.
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    // Now clear the bit.
    frames[index] &= ~(0x1 << offset);
}

// testFrame(uint32_t addr) - Test if a frame is set
uint32_t testFrame(uint32_t addr) {
    // Same as all previous functions, but we return a value.
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    return (frames[index] & (0x1 << offset)); // Return if the frame is set
}


// firstFrame() - Find the first free frame
uint32_t firstFrame() {
    // Loop through to find a free frame.
    for (uint32_t i = 0; i < INDEX_BIT(nframes); i++) {
        // Only continue if we have enough memory.
        if (frames[i] != 0xFFFFFFFF) {
            // Cool! We know at least one bit is free, let's test all 32 (8*4, remember?).
            for (uint32_t x = 0; x < 32; x++) {
                uint32_t test = 0x1 << x;
                if (!(frames[i] & test)) {
                    return i * 4 * 8 + x; // We did it, we found a frame!
                }
            }
        }
    }

}

// firstFrames(size_t n) - Finds n amount of frames and returns it.
uint32_t firstFrames(size_t n) {
    if (n == 0) return 0; // stupid users
    if (n == 1) return firstFrame();

    for (uint32_t i = 0; i < INDEX_BIT(nframes); i++) {
        if (frames[i] != 0xFFFFFFFF) {
            // Cool! We know at least one is free, lets check each bit.
            for (uint32_t j = 0; j < 32; j++) {
                int bit = frames[i] & (1 << j);
                if (!bit) {
                    int startingBit = i*32; // Get the free bit at index i
                    startingBit += bit;


                    uint32_t free = 0; // Check each bit to see if enough frames are present.
                    for (uint32_t count = 0; count < n; count++) {
                        if (!testFrame(startingBit + count)) free++; // Check me for bugs!
                        if (free == n) return i*4*8+j;
                        else serialPrintf("firstFrames: free != size when locating %i frames\n", n);
                    }
    
                }
            }
        }
    }
    return -1;
}




// BrokenThorn Entertainment's PMM architecture involves using memory blocks/regions. To get the architecture to work correctly, we need to do the same.
void pmm_initRegion(uint32_t base, size_t size) {
    int align = base / 4096;
    int blocks = size / 4096;

    for (; blocks > 0; blocks--) {
        clearFrame(align++);
        pmm_usedBlocks--;
    }

    setFrame(0); // First block is always set.
}

void pmm_deinitRegion(uint32_t base, size_t size) {
    int align = base / 4096;
    int blocks = size / 4096;

    serialPrintf("blocks used %i\n", blocks);
    for (; blocks > 0; blocks--) {
        setFrame(align++);
        pmm_usedBlocks++;
    }

    setFrame(0);

    serialPrintf("pmm_deinitRegion: Region at 0x%x (size 0x%x) deinitialized. Blocks used: %i. Free blocks: %i\n", base, size, blocks, (pmm_maxBlocks - pmm_usedBlocks));
}

void *pmm_allocateBlock() {
    if ((pmm_maxBlocks - pmm_usedBlocks) <= 0) return 0; // OOM

    int frame = firstFrame();
    if (frame == -1) return 0; // OOM

    setFrame(frame);

    uint32_t addr = frame * 4096;
    pmm_usedBlocks++;

    return (void*)addr;
}

void pmm_freeBlock(void *block) {
    uint32_t addr = (uint32_t)block;
    int frame = addr / 4096;

    clearFrame(frame);

    pmm_usedBlocks--;
}

void *pmm_allocateBlocks(size_t size) {
    if ((pmm_maxBlocks - pmm_usedBlocks) <= size) return 0; // Not enough memory is available
    int frame = firstFrames(size);
    if (frame == -1) {
        serialPrintf("pmm_allocateBlocks: Failed to allocate %i blocks\n", size);
        return 0;
    }

    for (uint32_t i=0; i < size; i++) {
        setFrame(frame + i);
    }

    uint32_t addr = frame * 4096;
    pmm_usedBlocks += size;
    return (void*)addr;
}


void pmm_freeBlocks(void *ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / 4096;

    for (uint32_t i = 0; i < size; i++) clearFrame(frame + i);

    pmm_usedBlocks -= size;
}

uint32_t pmm_getPhysicalMemorySize() {
    return pmm_memorySize;
}

uint32_t pmm_getMaxBlocks() {
    return pmm_maxBlocks;
}

uint32_t pmm_getUsedBlocks() {
    return pmm_usedBlocks;
}

uint32_t pmm_getFreeBlocks() {
    return (pmm_getMaxBlocks() - pmm_getUsedBlocks());
}