// ===================================================================
// paging_pde.c - Handles page directory entries.
// Provides an abstract interface to aid in management of PDEs
// ===================================================================
// This implementation is not by me. Credits in the header file.

#include "include/paging_pde.h" // Main header file


void pd_entry_add_attribute(pd_entry *e, uint32_t attribute) {
    *e |= attribute;
}

void pd_entry_del_attribute(pd_entry *e, uint32_t attribute) {
    *e &= ~attribute;
}

void pd_entry_set_frame(pd_entry *e, physicalAddress address) {
    *e = (*e & ~I86_PDE_FRAME) | address;
}

bool pd_entry_is_present(pd_entry e) {
    return e & I86_PDE_PRESENT;
}

bool pd_entry_is_writable(pd_entry e) {
    return e & I86_PDE_WRITABLE;
}

physicalAddress pd_entry_pfn(pd_entry e) {
    return e & I86_PDE_FRAME;
} 

bool pd_entry_is_user(pd_entry e) {
    return e & I86_PDE_USER;
}

bool pd_entry_is_4mb(pd_entry e) {
    return e & I86_PDE_4MB;
}

void pd_entry_enable_global(pd_entry e) {

}

