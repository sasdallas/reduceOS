// mem.h - A header file for the entire memory management system

#ifndef MEM_H
#define MEM_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/string.h> // String functions
#include <libk_reduced/assert.h>
#include <kernel/vmm.h>

#if defined(ARCH_I386) || defined(__INTELLISENSE__)
#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>
#else
#error "Unknown or unsupported architecture"
#endif

// Macros
#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)
#define PAGEDIR_INDEX(x) (((x) >> 22) & 0x3ff) // Returns the index of x within the PDE
#define PAGETBL_INDEX(x) (((x) >> 12) & 0x3ff) // Returns the index of x within the PTE
#define VIRTUAL_TO_PHYS(addr) (*addr & ~0xFFF) // Returns the physical address of addr.

// Flags for the architecture memory mapper
// By default, if created, a page will be given user-mode and write access.
#define MEM_CREATE              0x01    // Create the page. Commonly used with mem_getPage during mappings
#define MEM_KERNEL              0x02    // The page is kernel-mode only
#define MEM_READONLY            0x04    // The page is read-only
#define MEM_WRITETHROUGH        0x08    // The page is marked as writethrough
#define MEM_NOT_CACHEABLE       0x10    // The page is not cacheable
#define MEM_NOALLOC             0x20    // Do not allocate the page and instead use what was given
// PAT support will add more bits


// errno doesn't have specifications for MEM, and this is kernel only.
// These are the possible error values
#define MEM_ERR_PRESENT -1      // The page isn't present in memory



// Memory management functions
uintptr_t mem_getPhysicalAddress(pagedirectory_t *dir, uintptr_t virtaddr);
void mem_outofmemory(int pages, char *seq);



// Functions
void enable_liballoc();
void *kmalloc(size_t size);
void *krealloc(void *a, size_t b);
void *kcalloc(size_t a, size_t b);
void kfree(void *a);

#endif
