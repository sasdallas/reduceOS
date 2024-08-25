// ========================================================================
// liballoc_forwader.c - The embodyment of my hatred
// ======================================================================== 
// Unfortunately, this file is part of the reduceOS C kernel. If you're using this code, please, think about your life...

#include <kernel/mem.h> // Main header file

uint32_t placement_address = (uint32_t)&end;
int liballoc_enabled = 0;

// This is an abomination

void enable_liballoc() { liballoc_enabled = 1; }

void *kmalloc(size_t size){

    // Check if liballoc is enabled, and if so, use that instead.
    if (liballoc_enabled) {
        return liballoc_kmalloc(size);
    } else {
        // There's a *very* slim window when this could happen. Therefore, just place it at the end...
        // TODO: Fix this abomination.
        if ((placement_address && 0x00000FFF)) {
            placement_address = ALIGN_PAGE(placement_address);
        }

        uint32_t ret = placement_address;
        placement_address += size;
        return ret;
    }
}


void *krealloc(void *a, size_t b) {
    return liballoc_krealloc(a, b);
}

void *kcalloc(void *a, size_t b) {
    return liballoc_kcalloc(a, b);
}

void kfree(void *a) {
    return liballoc_kfree(a);
}

