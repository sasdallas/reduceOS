// page.h (i386 architecture specific)

#if !defined(ARCH_I386) && !defined(__INTELLISENSE__) 
#error "This file is for the i386 architecture" 
#endif

#ifndef ARCH_I386_PAGE_H
#define ARCH_I386_PAGE_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>

// Definitions
#define PAGE_SHIFT 12       // This is the amount to shift page addresses to get the index of the PTE

#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)
#define PAGEDIR_INDEX(x) (((x) >> 22) & 0x3ff) // Returns the index of x within the PDE
#define PAGETBL_INDEX(x) (((x) >> 12) & 0x3ff) // Returns the index of x within the PTE
#define VIRTUAL_TO_PHYS(addr) (*addr & ~0xFFF) // Returns the physical address of addr.

// Typedefs
// pagedirectory_t
typedef struct {
    pde_t entries[1024];
} pagedirectory_t;


// pagetable_t
typedef struct {
    pte_t entries[1024]; // x86 architecture specifies 1024 entries per page table 
} pagetable_t;

#endif 