// vmm.h - Header file for virtual memory manager

#ifndef VMM_H
#define VMM_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions

#include "include/panic.h" // Kernel panicking
#include "include/heap.h" // Kernel heap management.
#include "include/terminal.h"
#include "include/serial.h"
#include "include/pmm.h" // Physical memory management

#include "include/vmm_pte.h"
#include "include/vmm_pde.h"


// Typedefs
typedef uint32_t virtual_address;


// pagetable_t
typedef struct {
    pte_t entries[1024]; // x86 architecture specifies 1024 entries per page table 
} pagetable_t;

// pagedirectory_t
typedef struct {
    pde_t entries[1024];
} pagedirectory_t;

// Definitions
#define PAGE_SIZE 4096 // Page size


// Macros
#define PAGEDIR_INDEX(x) (((x) >> 22) & 0x3ff) // Returns the index of x within the PDE
#define PAGETBL_INDEX(x) (((x) >> 12) & 0x3ff) // Returns the index of x within the PTE
#define VIRTUAL_TO_PHYS(addr) (*x & ~0xFFF) // Returns the physical address of addr.


#endif