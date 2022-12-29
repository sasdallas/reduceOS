// paging_pte.h - Header file for paging_pte.c, manages page table entries
// This implementation is not by me, it is by BrokenThorn Entertainment.

#ifndef PAGING_PTE_H
#define PAGING_PTE_H

// Includes
#include "include/libc/stdint.h" // Integer types
#include "include/mem_phys.h" // Physical memory

// Typedefs
typedef enum {
    I86_PTE_PRESENT = 1,
    I86_PTE_WRITABLE = 2,
    I86_PTE_USER = 4,
    I86_PTE_WRITETHROUGH = 8,
    I86_PTE_NOT_CAHCEABLE = 0x10,
    I86_PTE_ACCESSED = 0x20,
    I86_PTE_DIRTY = 0x40,
    I86_PTE_PAT = 0x80,
    I86_PTE_CPU_GLOBAL = 0x100,
    I86_PTE_LV4_GLOBAL = 0x200,
    I86_PTE_FRAME = 0x7FFFFF000
} PAGE_PTE_FLAGS;

// Page table entry
typedef uint32_t pt_entry;

// Functions:
void pt_entry_add_attribute(pt_entry *e, uint32_t attribute);
void pt_entry_del_attribute(pt_entry *e, uint32_t attribute);
void pt_entry_set_frame(pt_entry *e, physicalAddress addr);
bool pt_entry_is_present(pt_entry e);
bool pt_entry_is_writable(pt_entry e);
physicalAddress pt_entry_pfn(pt_entry e);
#endif