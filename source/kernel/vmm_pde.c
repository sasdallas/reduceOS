// =====================================================
// vmm_pde.c - Page Directory Entries
// =====================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTICE: Paging file structure and design sourced from BrokenThorn Entertainment. MOST code is written by me, but PLEASE credit them!

#include "include/vmm_pde.h" // Main header file

// Most of this file is just PDE helper functions

// pde_addattrib(pde_t *entry, uint32_t attribute) - Add attribute
void pde_addattrib(pde_t *entry, uint32_t attribute) {
    *entry = *entry | attribute;
}

// pde_delattrib(pde_t *entry, uint32_t attribute) - Delete/clear attribute
void pde_delattrib(pde_t *entry, uint32_t attribute) {
    *entry = *entry & ~(attribute);
}

// pde_setframe(pde_t *entry, uint32_t physical_addr) - Set the PDE frame address
void pde_setframe(pde_t *entry, uint32_t physical_addr) {
    *entry = (*entry & ~PDE_FRAME) | physical_addr;
}

// pde_ispresent(pde_t entry) - Returns whether the PDE is present in memory
bool pde_ispresent(pde_t entry) {
    return entry & PDE_PRESENT; 
}

// pde_iswritable(pde_t entry) - Returns whether the PDE is writable
bool pde_iswritable(pde_t entry) {
    return entry & PDE_WRITABLE;
}

// pde_getframe(pde_t entry) - Returns the address of PDE_FRAME
uint32_t pde_getframe(pde_t entry) {
    return entry & PDE_FRAME;
}

// pde_isuser(pde_t entry) - Returns whether the PDE is usermode
bool pde_isuser(pde_t entry) {
    return entry & PDE_USER;
}

// pde_is4mb(pde_t entry) - Returns whether the PDE is 4MB pages
bool pde_is4mb(pde_t entry) {
    return entry & PDE_4MB;
}

