// =============================================================================
// liballoc_wrappper.c - A wrapper for liballoc, written by Durand Miller
// =============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTE: liballoc is NOT by me! It is not by blanham (creator of the repo linked in the README.MD), but rather by Durand Miller
// NO CODE OF LIBALLOC.C OR LIBALLOC.H IS CREATED BY ME.

#include <kernel/liballoc_wrapper.h> // Main header file
#include <libk_reduced/stdio.h>
#include <kernel/panic.h>

spinlock_t lock = SPINLOCK_RELEASED; // Memory lock (we dont use spinlock_init because ehhhh)

// liballoc requires four functions to be declared before it can operate - here they are:

extern uint32_t end;

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
        mem_outofmemory(pages + 1, "liballoc allocation");
    } 

    
    // Now, we can map the pages (basically identity mapping) to the ptr.
    // This is not a good idea, and will screw everything up.
    for (size_t i = 0; i < pages; i++) {
        vmm_allocateRegionFlags((uintptr_t)(ptr + (i*4096)), (uintptr_t)(ptr + (i*4096)), PAGE_SIZE, 1, 1, 0);
    }

    return ptr;
}

// liballoc_free(void *addr, size_t pages) - Frees pages
int liballoc_free(void *addr, size_t pages) {
    // The PMM is identity mapped for now, so we don't need to get phys. addr.
    if (addr) {
        for (uint32_t i = (uint32_t)addr; i < (uint32_t)(addr + (pages * PAGE_SIZE)); i += 0x1000) {
            vmm_allocateRegionFlags(i, i, 0x1000, 0, 0, 0);
        }

        pmm_freeBlocks(addr, pages * PAGE_SIZE);
    }

    return 0;
}
