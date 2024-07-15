// ========================================================================
// liballoc_forwader.c - The embodyment of my hatred
// ======================================================================== 

#include "include/liballoc_forwarder.h" // Main header file

uint32_t placement_address = (uint32_t)&end;
int liballoc_enabled = 0;


// Just a quick, dirty hack I came up with. 
// Allow me to explain - we're running back into an old friend/enemy, BIOS32.
// BIOS32 causes bugs in paging code and HAS to be called for VBE/VESA, which sucks because no one likes it.
// ESPECIALLY not my memory code! liballoc specifically causes a ton of issues with it, and I'm sick and tired of it.
// BIOS32 will be replaced with a v86 monitor eventually,  but for now we're just not going to bother with it.
// I hate my life.


void enable_liballoc() { liballoc_enabled = 1; }

uint32_t kmalloc(uint32_t size){

    // Check if liballoc is enabled, and if so, use that instead.
    if (liballoc_enabled) {
        liballoc_kmalloc(size);
    } else {
        // I'm going mentally insane
        if ((placement_address && 0x00000FFF)) {
            placement_address = ALIGN_PAGE(placement_address);
        }

        uint32_t ret = placement_address;
        placement_address += size;
        return ret;
    }
}


void *krealloc(void *a, size_t b) {
    if (liballoc_enabled) {
        return liballoc_krealloc(a, b);
    } else {
        serialPrintf("krealloc: Something tried to reallocate but we don't have liballoc running!\n");
    }
}

void *kcalloc(void *a, size_t b) {
    if (liballoc_enabled) {
        return liballoc_kcalloc(a, b);
    } else {
        serialPrintf("kcalloc: Something tried to call calloc but we don't have liballoc running!\n");
    }
}

void kfree(void *a) {
    if (liballoc_enabled) {
        return liballoc_kfree(a);
    } else {
        serialPrintf("kfree: Something tride to call kfree but we don't have liballoc running!\n");
    }
}

