// ===================================================================
// paging_pde.c - Handles page table entries.
// Provides an abstract interface to aid in management of PTEs
// ===================================================================
// This implementation is not by me. Credits in the header file.

#include "include/paging_pte.h" // Main header file

void pt_entry_add_attribute(pt_entry *e, uint32_t attribute) {
    *e |= attribute;
}

void pt_entry_del_attribute(pt_entry *e, uint32_t attribute) {
    *e &= ~attribute;
}

void pt_entry_set_frame(pt_entry *e, physicalAddress addr) {
    *e = (*e & ~I86_PTE_FRAME) | addr;
}

bool pt_entry_is_present(pt_entry e) {
    return e & I86_PTE_PRESENT;
}

bool pt_entry_is_writable(pt_entry e) {
    return e & I86_PTE_WRITABLE;
}

physicalAddress pt_entry_pfn(pt_entry e) {
    return e & I86_PTE_FRAME;
}