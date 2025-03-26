// ========================================================================
// liballoc_forwader.c - The embodyment of my hatred
// ======================================================================== 
// Unfortunately, this file is part of the reduceOS C kernel. If you're using this code, please, think about your life...

/* TODO: Check me for bugs! */

#include <kernel/mem.h> // Main header file
#include <kernel/serial.h>
#include <kernel/liballoc.h>

#pragma GCC diagnostic ignored "-Wunused-value"

extern uint32_t end;
extern bool pagingEnabled;

int liballoc_enabled = 0;

void enable_liballoc() { liballoc_enabled = 1; }

void *kmalloc(size_t size){

    // Check if liballoc is enabled, and if so, use that instead.
    if (liballoc_enabled) {
        void *out = liballoc_kmalloc(size);

        return out;
    } else {
        panic("reduceOS", "trap", "deprecated secondary kmalloc");
        __builtin_unreachable();
    }
}


void *krealloc(void *a, size_t b) {
    void *out = liballoc_krealloc(a, b);

    return out;
}

void *kcalloc(size_t a, size_t b) {
    void *out = liballoc_kcalloc(a, b);
    
    return out;
}

void kfree(void *a) {
    liballoc_kfree(a);
}

