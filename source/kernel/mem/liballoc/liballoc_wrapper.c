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
        panic_prepare();
        serialPrintf("*** The memory manager could not successfully allocate enough memory.\n");
        serialPrintf("*** Failed to allocate %i pages during liballoc allocation sequence.\n", pages + 1);

        printf("*** The system has run out of memory.\n");
        printf("\nThis error indicates that your system has fully run out of memory and can no longer continue its operation.\n");
        printf("Please either do not open many resource intensive applications, or potentially use a larger pagefile\n"); // Good luck with that 
        printf("There is also the potential of an operating system bug causing this. If you have the proper amount of RAM as detailed in reduceOS system requirements, start an issue on GitHub.\n");
        printf("\nStack dump:\n");
        panic_dumpStack(NULL);
        printf("\n");
        registers_t *reg = (registers_t*)((uint8_t*)&end);
        asm volatile ("mov %%ebp, %0" :: "r"(reg->ebp));
        reg->eip = NULL;
        panic_stackTrace(7, reg);

        disableHardwareInterrupts();
        asm volatile ("hlt");
        for (;;);
        __builtin_unreachable();
    } 

    
    // Now, we can map the pages (basically identity mapping) to the ptr.
    // This is not a good idea, and will screw everything up.
    for (size_t i = 0; i < pages; i++) {
        vmm_mapPage(ptr + (i*4096), ptr + (i*4096));
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
