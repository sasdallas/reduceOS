// stdlib.c - replacement for the standard C file stdlib.c. Contains (mainly) memory allocation functions.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#include "include/libc/stdlib.h" // Main header file

// This file works in tandem with heap.c! kmalloc() functions are present here, but if kheap is not 0, give control to heap.c

extern heap_t *kernelHeap;

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys) {
    if (kernelHeap) {
        // Hand over control to heap.c.
        return kmalloc_heap(size, align, phys);
    } else {
        // If alignment is 1, align the placement address to 4096.
        if (align == 1 && (placement_address & 0xFFFFF000)) {
            placement_address &= 0xFFFFF000;
            placement_address += 0x1000;
        }

        // If phys is not 0, set it to the physical address (without adding the size to it).
        if (phys) { *phys = placement_address; }
        
        uint32_t returnValue = placement_address;
        placement_address += size;
        return returnValue;
    }
}


// The following functions use this key on their names:
// a - align placement address
// p - return physical address
// ap - do both
// none - do neither
// They do not have function descriptions, since those are not required.

uint32_t kmalloc_a(uint32_t size) {
    return kmalloc_int(size, 1, 0); 
}

uint32_t kmalloc_p(uint32_t size, uint32_t *phys) {
    return kmalloc_int(size, 0, phys);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys) {
    return kmalloc_int(size, 1, phys);
}

uint32_t kmalloc(uint32_t size) {
    return kmalloc_int(size, 0, 0);
}

