// ==========================================================
// pmm.c - Handles physical bitmaps for memory management
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// Some code was sourced from JamesM's Kernel Development Tutorials. Please credit him as well.

#include <kernel/pmm.h> // Main header file
#include <stdio.h>

// (from JamesM's kernel dev tutorials)
#define INDEX_BIT(a) (a/(8*4))
#define OFFSET_BIT(a) (a%(8*4))

// Some bitsets of frames - used or free.
uint32_t* frames;
uint32_t nframes;


// Useful information for OOM errors.
uint32_t pmm_memorySize; // Physical memory size
uint32_t pmm_usedBlocks; // Used blocks of physical memory
uint32_t pmm_maxBlocks; // Total amount of blocks of physical memory

// Memory region types as strings
char* strMemoryTypes[] = {
	"Available",			// Available
	"Reserved",			    // Reserved
	"ACPI Reclaim",		    // ACPI Reclaim (todo: initialize this)
	"ACPI NVS Memory"		// ACPI NVS Memory
};

// pmm_printMemoryMap(multiboot_info *info) - Prints a memory map
void pmm_printMemoryMap(multiboot_info *info) {
    printf("Physical memory map:\n");
    serialPrintf("DUMPING PHYSICAL MEMORY MAP:\n");

    for (uint32_t i = 0; i < info->m_mmap_length; i += sizeof(memoryRegion_t)) {
        memoryRegion_t *region = (memoryRegion_t*)((info->m_mmap_addr) + i);
        printf("\tRegion %i: size: %d address: 0x%x%x length: %d%d bytes type: %d (%s)\n", i, region->size, region->addressLo, region->addressHi, region->lengthLo, region->lengthHi, region->type, strMemoryTypes[region->type-1]);
        serialPrintf("\tRegion %i: size: %d address: 0x%x%x length: %d%d bytes type: %d (%s)\n", i, region->size, region->addressLo, region->addressHi, region->lengthLo, region->lengthHi, region->type, strMemoryTypes[region->type-1]);
    }
}

// pmm_initializeMemoryMap(multiboot_info *info) - Initializes available memory sections in the memory map
void pmm_initializeMemoryMap(multiboot_info *info) {
    for (uint32_t i = 0; i < info->m_mmap_length; i += sizeof(memoryRegion_t)) {
        memoryRegion_t *region = (memoryRegion_t*)((info->m_mmap_addr) + i);
        if (region->type == 1) pmm_initRegion(region->addressLo, region->lengthLo);
    }
}

// pmmInit(uint32_t physMemorySize) - Initializes frames and nframes
void pmmInit(uint32_t physMemorySize) {
    pmm_memorySize = physMemorySize;
    pmm_maxBlocks = (pmm_memorySize * 1024) / 4096;
    pmm_usedBlocks = pmm_maxBlocks;

    nframes = pmm_maxBlocks;
    frames = kmalloc(nframes);  // TODO: Find a better way to do this, liballoc forwarder will toss it into a page near the heap.
                                // This page is never kept track of and QUITE important, since it holds the core allocation framework.
                                // Maybe map to &end and just deinitialize region?
                                
    // All memory is in use by default.
    memset(frames, 0xF, pmm_maxBlocks / 8);
}


// pmm_setFrame(int frame) - Set a frame/bit in the frames array
void pmm_setFrame(int frame) {
    frames[INDEX_BIT(frame)] |= (1 << OFFSET_BIT(frame));
}

// pmm_clearFrame(int frame) - Clear a frame/bit in the frames array
void pmm_clearFrame(int frame) {
    frames[INDEX_BIT(frame)] &= ~(1 << OFFSET_BIT(frame));
}

bool pmm_testFrame(int frame) {
    return frames[INDEX_BIT(frame)] & (1 << OFFSET_BIT(frame));
}


// setFrame(uint32_t addr) - Setting a bit in the frames bitset. (stupid paging code OBSOLETE)
void setFrame(uint32_t addr) {
    uint32_t frame = addr / 4096; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    frames[index] |= (0x1 << offset);
}


// clearFrame(uint32_t addr) - Clear a bit in the frames bitset (stupid paging code OBSOLETE)
void clearFrame(uint32_t addr) {
    // Same as setFrame, but with a few differences.
    uint32_t frame = addr / 4096; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    // Now clear the bit.
    frames[index] &= ~(0x1 << offset);
}

// testFrame(uint32_t addr) - Test if a frame is set (stupid paging code OBSOLETE)
uint32_t testFrame(uint32_t addr) {
    // Same as all previous functions, but we return a value.
    uint32_t frame = addr / 4096; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    return (frames[index] & (0x1 << offset)); // Return if the frame is set
}


// pmm_firstFrame() - Find the first free frame
uint32_t pmm_firstFrame() {
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

    return -1;

}

// pmm_firstFrames(size_t n) - Finds n amount of frames and returns it.
uint32_t pmm_firstFrames(size_t n) {
    if (n == 0) return 0; // stupid users
    if (n == 1) return pmm_firstFrame();

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
                        if (!pmm_testFrame(startingBit + count)) free++;
                        if (free == n) return i*4*8+j;
                    }
    
                }
            }
        }
    }
    return -1;
}




// pmm_initRegion(uintptr_t base, size_t size) - Initialize a region as available memory
void pmm_initRegion(uintptr_t base, size_t size) {
    int align = base / 4096;
    int blocks = size / 4096;

    for (; blocks > 0; blocks--) {
        pmm_clearFrame(align++);
        pmm_usedBlocks--;
    }

    pmm_setFrame(0); // First block is always set.

    serialPrintf("pmm_initRegion: Region at 0x%x (size 0x%x) initialized. Blocks used: %i. Free blocks: %i\n", base, size, blocks, (pmm_maxBlocks - pmm_usedBlocks));
}

// pmm_deinitRegion(uintptr_t base, size_t size) - Marks a region as unusable memory (e.g. kernel region)
void pmm_deinitRegion(uintptr_t base, size_t size) {
    // maybe bugged? dunno
    int align = base / 4096;
    int blocks = size / 4096;

    for (; blocks > 0; blocks--) {
        pmm_setFrame(align++);
        pmm_usedBlocks++;
    }

    pmm_setFrame(0);

    serialPrintf("pmm_deinitRegion: Region at 0x%x (size 0x%x) deinitialized. Blocks used: %i. Free blocks: %i\n", base, size, pmm_usedBlocks, (pmm_maxBlocks - pmm_usedBlocks));
}

// pmm_allocateBlock() - Allocates a block
void *pmm_allocateBlock() {
    if ((pmm_maxBlocks - pmm_usedBlocks) <= 0) {
        serialPrintf("pmm_allocateBlock: Out of memory!\n");
        return 0; // OOM
    }

    int frame = pmm_firstFrame();
    if (frame == -1) {
        serialPrintf("pmm_allocateBlock: Failed to allocate block.\n");
        return 0; // OOM
    }

    pmm_setFrame(frame);

    uint32_t addr = frame * 4096;
    pmm_usedBlocks++;

    return (void*)addr;
}

// pmm_freeBlock(void *block) - Frees a block
void pmm_freeBlock(void *block) {
    uint32_t addr = (uint32_t)block;
    int frame = addr / 4096;

    pmm_clearFrame(frame);

    pmm_usedBlocks--;
}

// pmm_allocateBlocks(size_t size) - Allocates size amount of blocks
void *pmm_allocateBlocks(size_t size) {
    if (size > 4096) {
        serialPrintf("pmm_allocateBlocks: Warning, a potential block overrun might happen - size is 0x%x\n", size);
        panic("pmm", "pmm_allocateBlocks", "A function may have attempted to pass in bytes instead of blocks.");
    }

    if ((pmm_maxBlocks - pmm_usedBlocks) <= size) {
        serialPrintf("pmm_allocateBlocks: Out of memory trying to allocate 0x%x blocks\n", size);
        return 0; // Not enough memory is available
    }

    int frame = pmm_firstFrames(size);
    if (frame == -1) {
        serialPrintf("pmm_allocateBlocks: Failed to allocate %i blocks\n", size);
        return 0;
    }

    for (uint32_t i=0; i < size; i++) {
        pmm_setFrame(frame + i);
    }

    uint32_t addr = frame * 4096;
    pmm_usedBlocks += size;
    return (void*)addr;
}

// pmm_freeBlocks(void *ptr, size_t size) - Frees blocks in memory
void pmm_freeBlocks(void *ptr, size_t size) {
    uint32_t addr = (uint32_t)ptr;
    int frame = addr / 4096;

    for (uint32_t i = 0; i < size; i++) pmm_clearFrame(frame + i);

    pmm_usedBlocks -= size;
}


// Getter functions
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
