// =============================================================================
// liballoc_wrappper.c - A wrapper for liballoc, written by Durand Miller
// =============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTE: liballoc is NOT by me! It is not by blanham (creator of the repo linked in the README.MD), but rather by Durand Miller
// NO CODE OF LIBALLOC.C OR LIBALLOC.H IS CREATED BY ME.

#include "include/liballoc_wrapper.h" // Main header file

atomic_flag lock = ATOMIC_FLAG_INIT; // Memory lock (we dont use spinlock_init because ehhhh)

// liballoc requires four functions to be declared before it can operate - here they are:


// liballoc_lock() - Locks the memory data structures (TODO: Should we disable interrupts? Idk if this spinlock is doing anything lol)
int liballoc_lock() {
    spinlock_lock(&lock);
    return 0;
}

// liballoc_unlock() - Unlocks the memory data structures
int liballoc_unlock() {
    spinlock_release(&lock);
    return 0;
}

// liballoc_alloc(size_t pages) - Allocates pages
void *liballoc_alloc(size_t pages) {
    // Allocate some physical memory for the pages
    void *ptr = pmm_allocateBlocks(pages + 1); // one extra to stop stupid liballoc from doing stupid things
    if (!ptr) {
        return  NULL;
    } 

    
    // Now, we can map the pages (basically identity mapping) to the ptr.
    for (int i = 0; i < pages; i++) {
        vmm_mapPage(ptr + (i*4096), ptr + (i*4096));
    }


    return ptr;
}

// liballoc_free(void *addr, size_t pages) - Frees pages
int liballoc_free(void *addr, size_t pages) {
    // Free the blocks of memory first
    pmm_freeBlocks(addr, pages);

    // Get the page directory for our virtual address
    pde_t *entry = vmm_getPageTable((uint32_t)addr);
    pagetable_t *pageTable = (pagetable_t*)VIRTUAL_TO_PHYS(entry);

    // Now, we iterate through the table and find anything relating to our frames
    for (int i = 0; i < pages; i++) {
        pte_t *entry = vmm_tableLookupEntry(pageTable, addr + (i*4096));
        vmm_freePage(entry);
    }

    return 0;
}
