// vmm_pde.h - Virtual memory manager page directory entries

#ifndef VMM_PDE_H
#define VMM_PDE_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations

typedef uint32_t pde_t; // See vmm_pte.h 

#include <libk_reduced/string.h> // Will cause include loop

// Definitions
#define PAGETABLE_ADDRSPACE 0x400000 // 4 MB address space

// Enums

// PDE flags
enum PAGING_PDE_FLAGS {
    PDE_PRESENT = 1,
    PDE_WRITABLE = 2,
    PDE_USER = 4,
    PDE_PWT = 8,
    PDE_PCD = 0x10,
    PDE_ACCESSED = 0x20,
    PDE_DIRTY = 0x40,
    PDE_4MB = 0x80,
    PDE_CPU_GLOBAL = 0x100,
    PDE_LV4_GLOBAL = 0x200,
    PDE_FRAME = 0x7FFFF000
};


// Functions
void pde_addattrib(pde_t *entry, uint32_t attribute);
void pde_delattrib(pde_t *entry, uint32_t attribute);
void pde_setframe(pde_t *entry, uint32_t physical_addr);
bool pde_ispresent(pde_t entry);
bool pde_iswritable(pde_t entry);
uint32_t pde_getframe(pde_t entry);
bool pde_isuser(pde_t entry);
bool pde_is4mb(pde_t entry);


#endif
