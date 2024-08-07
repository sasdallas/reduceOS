// ==========================================================
// vmm_pte.c - Page Table Entry handler
// ==========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTICE: Paging file structure and design sourced from BrokenThorn Entertainment. MOST code is written by me, but PLEASE credit them!

#include <kernel/vmm_pte.h> // Main header file

// Most of this file is just PTE helper functions.

// pte_addattrib(pte_t *entry, uint32_t attribute) - Add an attribute to the PTE.
void pte_addattrib(pte_t *entry, uint32_t attribute) {
    *entry |= attribute;
}

// pte_delattrib(pte_t *entry, uint32_t attribute) - Delete an attribute from the PTE.
void pte_delattrib(pte_t *entry, uint32_t attribute) {
    *entry &= ~attribute;
}

// pte_setframe(pte_t *entry, uint32_t physical_addr) - Set the PTE frame.
void pte_setframe(pte_t *entry, uint32_t physical_addr) {
    *entry = (*entry & ~PTE_FRAME) | physical_addr;
}

// pte_ispresent(pte_t entry) - Returns whether the PTE is in memory or not.
bool pte_ispresent(pte_t entry) {
    return entry & PTE_PRESENT;
}

// pte_iswritable(pte_t entry) - Returns whether the PTE is writable.
bool pte_iswritable(pte_t entry) {
    return entry & PTE_WRITABLE;
}

// pte_getframe(pte_t entry) - Returns the address of the PTE frame.
uint32_t pte_getframe(pte_t entry) {
    return entry & PTE_FRAME;
}
