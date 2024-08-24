// vmm.h - Header file for virtual memory manager

#ifndef VMM_H
#define VMM_H

// Includes
#include <stdint.h> // Integer declarations
#include <string.h> // String functions


#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>

#include <kernel/panic.h> // Kernel panicking
#include <kernel/heap.h> // Kernel heap management.
#include <kernel/terminal.h>
#include <kernel/serial.h>
#include <kernel/pmm.h> // Physical memory management



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
#define VIRTUAL_TO_PHYS(addr) (*addr & ~0xFFF) // Returns the physical address of addr.

// Functions
pte_t *vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr); // Look up an entry within the page table
pde_t *vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr); // Look up an entry within the page directory
void vmm_loadPDBR(uint32_t pdbr_addr); // Loads a new value into the PDBR address
bool vmm_switchDirectory(pagedirectory_t *directory); // Changes the current page directory
void vmm_flushTLBEntry(uint32_t addr); // Invalidates the current TLB entry
pagedirectory_t *vmm_getCurrentDirectory(); // Returns the current page directory
bool vmm_allocatePage(pte_t *entry); // Allocates VMM pages
void vmm_freePage(pte_t *entry); // Frees a page
pde_t *vmm_getPageTable(void *virtual_address); // Returns the page table for a virtual address
void vmm_mapPage(void *physical_addr, void *virtual_addr); // Maps a page from the physical address to its virtual address
void vmm_enablePaging(); // Enables paging
void vmm_disablePaging(); // Disables paging
void vmm_allocateRegion(uintptr_t physical_address, uintptr_t virtual_address, size_t size); // Identity map a region
void vmmInit(); // Initialize the VMM
void vmm_allocateRegionFlags(uintptr_t physical_address, uintptr_t virtual_address, size_t size, int present, int writable, int user); // Identity map region with flags
int vmm_createPageTable(pagedirectory_t *dir, uint32_t virt, uint32_t flags);
void *vmm_getPhysicalAddress(pagedirectory_t *dir, uint32_t virt);
void vmm_mapPhysicalAddress(pagedirectory_t *dir, uint32_t virt, uint32_t phys, uint32_t flags);
pagedirectory_t *vmm_createAddressSpace();
void vmm_unmapPageTable(pagedirectory_t *dir, uint32_t virt);
void vmm_unmapPhysicalAddress(pagedirectory_t *dir, uint32_t virt);


#endif
