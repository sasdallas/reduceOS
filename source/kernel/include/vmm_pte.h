// vmm_pte.h - header file for the VMM page table entry handler

#ifndef VMM_PTE_H
#define VMM_PTE_H


// We get an include loop if we include string.h before defining pte_t.

#include <stdint.h> // Integer declarations

// Typedefs
typedef uint32_t pte_t;


// Includes
#include <string.h>

// Definitions
#define PAGEDIR_ADDRSPACE 0x100000000 // 4 GB address space

// Enums
enum PTE_FLAGS {
    PTE_PRESENT = 1,
    PTE_WRITABLE = 2,
    PTE_USER = 4,
    PTE_WRITETHROUGH = 8,
    PTE_NOT_CACHEABLE = 0x10,
    PTE_ACCESSED = 0x20,
    PTE_DIRTY = 0x40,
    PTE_PAT = 0x80,
    PTE_CPU_GLOBAL = 0x100,
    PTE_LV4_GLOBAL = 0x200,
    PTE_FRAME = 0x7FFFF000
};


// Functions
void pte_addattrib(pte_t *entry, uint32_t attribute);
void pte_delattrib(pte_t *entry, uint32_t attribute);
void pte_setframe(pte_t *entry, uint32_t physical_addr);
bool pte_ispresent(pte_t entry);
bool pte_iswritable(pte_t entry);
uint32_t pte_getframe(pte_t entry);
#endif
