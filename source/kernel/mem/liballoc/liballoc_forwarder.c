// ========================================================================
// liballoc_forwader.c - The embodyment of my hatred
// ======================================================================== 
// Unfortunately, this file is part of the reduceOS C kernel. If you're using this code, please, think about your life...

/* TODO: Check me for bugs! */

#include <kernel/mem.h> // Main header file
#include <kernel/serial.h>
#include <kernel/liballoc.h>
#include <kernel/debug.h>

#pragma GCC diagnostic ignored "-Wunused-value"

extern uint32_t end;
extern bool pagingEnabled;

uint32_t placement_address = (uint32_t)&end;
int liballoc_enabled = 0;


void enable_liballoc() { liballoc_enabled = 1; }


void *kmalloc(size_t size){

    // Check if liballoc is enabled, and if so, use that instead.
    if (liballoc_enabled) {
        void *out = liballoc_kmalloc(size);

        heavy_dprintf("kernel allocate %i bytes to 0x%x\n", size, out);

        return out;
    } else {
        panic("reduceOS", "trap", "deprecated secondary kmalloc");

        // There's a *very* slim window when this could happen. Therefore, just place it at the end...
        // TODO: Fix this abomination.
        if ((placement_address & 0x00000FFF)) {
            placement_address = MEM_ALIGN_PAGE(placement_address);
        }

        uint32_t ret = placement_address;
        placement_address += size;
        return (void*)ret;
    }
}


void *krealloc(void *a, size_t b) {
    void *out =  liballoc_krealloc(a, b);

    heavy_dprintf("kernel reallocate %i bytes from 0x%x to 0x%x\n", b, a, out);

    return out;
}

void *kcalloc(size_t a, size_t b) {
    void *out = liballoc_kcalloc(a, b);
    
    heavy_dprintf("kernel calloc %i bytes %i times to 0x%x\n", a, b, out);
    
    return out;
}

void kfree(void *a) {
    liballoc_kfree(a);
    heavy_dprintf("kernel free ptr 0x%x\n", a);
}

