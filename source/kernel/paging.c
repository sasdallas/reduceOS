// ==========================================================
// paging.c - Manages kernel virtual memory
// ==========================================================
// This file is a part of the reduceOS C kernel. If you use this code, please credit the implementation's original owner (if you directly copy and paste this, also add me please)
// This implementation is not by me - credits for the implementation are present in paging.h


#include "include/paging.h" // Main header file

page_directory *kernelDir = 0; // The kernel's page directory
page_directory *currentDir = 0; // The current page directory

// Some bitsets of frames - used or free.
uint32_t *frames;
uint32_t nframes;


// Moving on to the functions...

// Starting off with static functions...

// setFrame(uint32_t addr) - Setting a bit in the frames bitset.
static void setFrame(uint32_t addr) {
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    frames[index] |= (0x1 << offset);
}


// clearFrame(uint32_t addr) - Clear a bit in the frames bitset
static void clearFrame(uint32_t addr) {
    // Same as setFrame, but with a few differences.
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    // Now clear the bit.
    frames[index] &= ~(0x1 << offset);
}

// testFrame(uint32_t addr) - Test if a frame is set
static uint32_t testFrame(uint32_t addr) {
    // Same as all previous functions, but we return a value.
    uint32_t frame = addr / PAGE_ALIGN; // Get the frame's actual address (aligned by 4k)
    uint32_t index = INDEX_BIT(frame); // Get the index and offset of the frame.
    uint32_t offset = OFFSET_BIT(frame);

    return (frames[index] & (0x1 << offset)); // Return if the frame is set
}


// firstFrame() - Find the first free frame
static uint32_t firstFrame() {
    // Loop through to find a free frame.
    for (uint32_t i = 0; i < INDEX_BIT(nframes); i++) {
        // Only continue if we have enough memory.
        if (frames[i] != 0xFFFFFFFF) {
            // Cool! We know at least one bit is free, let's test all 32 (8*4, remember?).
            for (uint32_t x = 0; x < 32; x++) {
                uint32_t test = 0x1 << x;
                if (!(frames[i] & test)) {
                    return i*4*8+x; // We did it, we found a frame!
                }
            }
        }
    }

}


// Now moving on to the functions all C files outside will probably use...

// allocateFrame(page *page, int kernel, int writable) - Allocating a frame.
void allocateFrame(page *page, int kernel, int writable) {
    if (page->frame != 0) return; // It's not valid if the frame is 0.
    else {
        uint32_t index = firstFrame(); // Get the index of the first free frame
        if (index == (uint32_t)-1) {
            // Panic! No free frames!
            panic("Paging", "allocateFrame()", "no free frames available!");
        }

        setFrame(index * 0x1000); // Set the frame, aligned to 4k
    
        // Setup all the page flags
        page->present = 1;
        page->rw == (writable) ? 1 : 0; // The caller of this function provides us whether the page is rw or ro.
        page->user = (kernel) ? 0 : 1; // Again, the caller of this function provides us whether the page is kernel.
        page->frame = index; // Set the page's frame value to the index of the first free frame.
    }
}


// freeFrame(page *page) - Deallocate (or "free") a frame.
void freeFrame(page *page) {
    uint32_t frame;
    if (!(frame = page->frame)) return; // Invalid frame.

    clearFrame(frame); // Clear the frame from the bitmap
    page->frame = 0x0; // Clear the frame value in the page.
}

// initPaging(uint32_t physicalMemorySize) - Initialize paging
void initPaging(uint32_t physicalMemorySize, uint32_t kernelSize) {
    // The user provides us with the size of physical memory.
    
    // Set nframes and frames to their proper values.
    nframes = physicalMemorySize / 0x1000;
    frames = (uint32_t*)allocateBlocks(INDEX_BIT(nframes));
    memset(frames, 0, INDEX_BIT(nframes));
    
    // Make a page directory.
    kernelDir = (page_directory*)allocateBlocks(sizeof(page_directory)); // Allocate a page directory for the kernel...
    currentDir = kernelDir; // We start in the kernel's page directory.

    // Now identity map (physical address to virtual address) from 0x0 to end of memory.
    // This is required so we can access this transparently, as if paging wasn't enabled.
    int i = 0; 
    while (i < kernelSize) {
        // The kernel code is read-only from userspace.
        allocateFrame(getPage(i, 1, kernelDir), 0, 0); 
        i += 0x1000;
    }
    

    isrRegisterInterruptHandler(14, pageFault); // pageFault is defined in panic.c
    switchPageDirectory(kernelDir); // Switch the page directory to kernel directory.
}

// switchPageDirectory(page_directory *dir) - Switches the page directory using inline assembly
void switchPageDirectory(page_directory *dir) {
    currentDir = dir;
    asm volatile ("mov %0, %%cr3" :: "r"(&dir->tablePhysical));
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!!
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}



// getPage(uint32_t addr, int make, page_directory *dir) - Returns a page from an address, directory (creates one if make is non-zero).
page *getPage(uint32_t addr, int make, page_directory *dir) {
    addr /= PAGE_ALIGN; // Turn the address into an index (aligned to 4096).
     
    // Find the page table containing this address.
    uint32_t tableIndex = addr / 1024; 
    if (dir->tables[tableIndex]) {
        // If this table is already assigned...
        return &dir->tables[tableIndex]->pages[addr % 1024];
    } else if (make) {
        uint32_t tmp = allocateBlocks(sizeof(page_table));
        dir->tables[tableIndex] = (page_table*)(tmp / PAGE_ALIGN);
        dir->tablePhysical[tableIndex] = tmp | 0x7; // Present, read-write, user-mode.
        return &dir->tables[tableIndex]->pages[addr%1024];
    } else { return 0; } // Nothing we can do.
}


