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

// pmmInit(uint32_t physMemorySize) - Initializes frames and nframes
void pmmInit(uint32_t physMemorySize) {
    nframes = physMemorySize / PAGE_SIZE; // Calculate number of frames

    frames = (uint32_t*)kmalloc(INDEX_BIT(nframes));
    memset(frames, 0, INDEX_BIT(nframes));
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

// allocateFrame(page_t *page, int kernel, int writable) - Allocating a frame.
void allocateFrame(page_t* page, int kernel, int writable) {
    if (page->frame != 0)
    {
        return;
    }
    else
    {
        uint32_t idx = firstFrame();
        if (idx == (uint32_t)-1) {
            panic("pmm", "allocateFrame()", "No free frames available!");
        }
        setFrame(idx * 0x1000);
        page->present = 1;
        page->rw = (writable) ? 1 : 0;
        page->user = (kernel) ? 0 : 1;
        page->frame = idx;
    }
}


// freeFrame(page_t *page) - Deallocate (or "free") a frame.
void freeFrame(page_t* page) {
    uint32_t frame;
    if (!(frame = page->frame)) return; // Invalid frame.

    clearFrame(frame); // Clear the frame from the bitmap
    page->frame = 0x0; // Clear the frame value in the page.
}

