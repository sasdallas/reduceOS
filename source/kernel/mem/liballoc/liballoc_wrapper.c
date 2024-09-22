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
    // sbrk ourselves
    serialPrintf("need to allocate %i pages\n", pages);
    void *addr = mem_sbrk(pages * PAGE_SIZE);
    memset(addr, 0x0, pages * PAGE_SIZE);
    return addr;
}

// liballoc_free(void *addr, size_t pages) - Frees pages
int liballoc_free(void *addr, size_t pages) {
    // Un-sbrk ourselves
    mem_sbrk(-(pages * PAGE_SIZE));

    return 0;
}