// paging_pde.h - Handles page directory entries (PDEs)
// This implementation is by BrokenThorn Entertainment, modified to support C.
// Please credit them if you use this code.

#ifndef PAGING_PDE_H
#define PAGING_PDE_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/mem_phys.h" // Physical memory management.

// Typedefs

// Defined by the x86 architecture.
typedef enum {
    I86_PDE_PRESENT = 1,
    I86_PDE_WRITABLE = 2,
    I86_PDE_USER = 4,
    I86_PDE_PWT = 8,
    I86_PDE_PCD = 0x10,
    I86_PDE_ACCESSED = 0x20,
    I86_PDE_DIRTY = 0x40,
    I86_PDE_4MB = 0x80,
    I86_PDE_CPU_GLOBAL = 0x100,
    I86_PDE_LV4_GLOBAL = 0x200,
    I86_PDE_FRAME = 0x7FFFF000
} PAGE_PDE_FLAGS;

// Page directory entry.
typedef uint32_t pd_entry;

// Functions:
void pd_entry_add_attribute(pd_entry *e, uint32_t attribute); // Sets a flag in the page table entry
void pd_entry_del_attribute(pd_entry *e, uint32_t attribute); // Clears a flag in the page table entry
void pd_entry_set_frame(pd_entry *e, physicalAddress address); // Sets a frame to a page table entry
bool pd_entry_is_present(pd_entry e); // Test if page is present
bool pd_entry_is_writable(pd_entry e); // Test if page is writable
physicalAddress pd_entry_pfn(pd_entry e); // Get page table entry frame address
bool pd_entry_is_user(pd_entry e); // Test if directory is in user mode
bool pd_entry_is_4mb(pd_entry e); // Test if directory contains 4 mb pages
void pd_entry_enable_global(pd_entry e); // Enable global pages


#endif