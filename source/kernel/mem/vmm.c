// ==================================================================
// vmm.c - 86th time's the charm, right? (Virtual Memory Mangement)
// ==================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTICE: Paging file structure and design sourced from BrokenThorn Entertainment. MOST code is written by me, but PLEASE credit them!

#include <kernel/vmm.h> // Main header file


#include <kernel/panic.h> // Kernel panicking

// Variables
pagedirectory_t *currentDirectory = 0; // Current page directory 
uint32_t currentPDBR = 0; // Current page directory base register address, unused.
bool pagingEnabled = false;


#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)
#define PAGEDIR_INDEX(x) (((x) >> 22) & 0x3ff) // Returns the index of x within the PDE
#define PAGETBL_INDEX(x) (((x) >> 12) & 0x3ff) // Returns the index of x within the PTE
#define VIRTUAL_TO_PHYS(addr) (*addr & ~0xFFF) // Returns the physical address of addr.


// vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) - Look up an entry within the page table.
pte_t *vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) {
    panic("VMM", "vmm_tableLookupEntry", "Deprecated");
    __builtin_unreachable();
}

// vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) - Look up an entry within the page directory.
pde_t *vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) {
    panic("VMM", "vmm_directoryLookupEntry", "Deprecated");
    __builtin_unreachable();
}

// vmm_loadPDBR(uint32_t pdbr_addr) - Loads a new value into the PDBR address
void vmm_loadPDBR(uint32_t pdbr_addr) {
    panic("VMM", "vmm_loadPDBR", "Deprecated");
    __builtin_unreachable();
}

// vmm_switchDirectory(pagedirectory_t *directory) - Changes the current page directory.
bool vmm_switchDirectory(pagedirectory_t *directory) {
    currentDirectory = directory;
    return true;
}


// vmm_flushTLBEntry(uint32_t addr) - Invalidate the current TLB entry
void vmm_flushTLBEntry(uint32_t addr) {
    panic("VMM", "vmm_flushTLBEntry", "Deprecated");
    __builtin_unreachable();
}

// vmm_getCurrentDirectory() - Returns the current page directory
pagedirectory_t *vmm_getCurrentDirectory() {
    return currentDirectory;
}

// vmm_allocatePage(pte_t *entry) - Allocate VMM pages
bool vmm_allocatePage(pte_t *entry) {
    panic("VMM", "vmm_allocatePage", "Deprecated");
    __builtin_unreachable();
}

// vmm_freePage() - Frees a page.
void vmm_freePage(pte_t *entry) {
    panic("VMM", "vmm_freePage", "Deprecated");
    __builtin_unreachable();
}

// vmm_getPageTable(void *virtual_address) - Returns the page table
pde_t *vmm_getPageTable(void *virtual_address) {
    panic("VMM", "vmm_getPageTable", "Deprecated");
    __builtin_unreachable();
}

// vmm_mapPage(void *physical_addr, void *virtual_addr) - Maps a page from the physical address to its virtual address
void vmm_mapPage(void *physical_addr, void *virtual_addr) {
    panic("VMM", "vmm_mapPage", "Deprecated");
    __builtin_unreachable();
}

// vmm_getPage(void *virtual_address) - Returns a page for a virtual address
pte_t *vmm_getPage(void *virtual_address) {
    panic("VMM", "vmm_getPage", "Deprecated");
    __builtin_unreachable();
}

// vmm_enablePaging() - Turns on paging
void vmm_enablePaging() {
    panic("VMM", "vmm_enablePaging", "Deprecated");
    __builtin_unreachable();
}


// vmm_disablePaging() - Disables paging
void vmm_disablePaging() {
    panic("VMM", "vmm_disablePaging", "Deprecated");
    __builtin_unreachable();
}


// vmm_allocateRegionFlags(uintptr_t physical_address, uintptr_t virtual_address, size_t size, int present, int writable, int user) - Identity map a region
void vmm_allocateRegionFlags(uintptr_t physical_address, uintptr_t virtual_address, size_t size, int present, int writable, int user) {
    panic("VMM", "vmm_allocateRegionFlags", "Deprecated");
}

// vmm_allocateRegion(uintptr_t physical_address, uintptr_t virtual_address, size_t size) - Identity map a region
void vmm_allocateRegion(uintptr_t physical_address, uintptr_t virtual_address, size_t size) {
    panic("VMM", "vmm_allocateRegion", "Deprecated");
}

// vmm_getPhysicalAddress(pagedirectory_t *dir, uint32_t virt) - Returns the physical address of a virtual address from a specific address space
void *vmm_getPhysicalAddress(pagedirectory_t *dir, uint32_t virt) {
    panic("VMM", "vmm_getPhysicalAddress", "Deprecated");
    __builtin_unreachable();
}

// vmm_mapPhysicalAddress(pagedirectory_t *dir, uint32_t virt, uint32_t phys, uint32_t flags) - Maps a physical address to a virtual address
void vmm_mapPhysicalAddress(pagedirectory_t *dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    panic("VMM", "vmm_mapPhysicalAddress", "Deprecated");
    __builtin_unreachable();
}

// vmm_createPageTable(pagedirectory_t *dir, uint32_t virt, uint32_t flags) - Creates a page table
int vmm_createPageTable(pagedirectory_t *dir, uint32_t virt, uint32_t flags) {
    panic("VMM", "vmm_createPageTable", "Deprecated");
    __builtin_unreachable();
}

// vmm_createAddressSpace() - Create an address space
pagedirectory_t *vmm_createAddressSpace() {
    panic("VMM", "vmm_createAddressSpace", "Deprecated");
    __builtin_unreachable();
}

// vmm_unmapPageTable(pagedirectory_t *dir, uint32_t virt) - Unmaps a page table
void vmm_unmapPageTable(pagedirectory_t *dir, uint32_t virt) {
    panic("VMM", "vmm_unmapPageTable", "Deprecated");
    __builtin_unreachable();
}

// vmm_unmapPhysicalAddress(pagedirectory_t *dir, uint32_t virt) - Unmaps a physical address
void vmm_unmapPhysicalAddress(pagedirectory_t *dir, uint32_t virt) {
    panic("VMM", "vmm_unmapPhysicalAddress", "Deprecated");
    __builtin_unreachable();
}



// vmmInit() - Initialize the VMM.
void vmmInit() {
    panic("VMM", "vmmInit", "Deprecated");
    __builtin_unreachable();
}

