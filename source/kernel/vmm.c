// ==================================================================
// vmm.c - 86th time's the charm, right? (Virtual Memory Mangement)
// ==================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// NOTICE: Paging file structure and design sourced from BrokenThorn Entertainment. MOST code is written by me, but PLEASE credit them!

#include "include/vmm.h" // Main header file

// Variables
pagedirectory_t *currentDirectory = 0; // Current page directory 
uint32_t currentPDBR = 0; // Current page directory base register address

// vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) - Look up an entry within the page table.
pte_t *vmm_tableLookupEntry(pagetable_t *table, uint32_t virtual_addr) {
    if (table) return &table->entries[PAGETBL_INDEX(virtual_addr)];
    else serialPrintf("vmm_tableLookupEntry: Invalid page table detected.\n");
    return 0;
}

// vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) - Look up an entry within the page directory.
pde_t *vmm_directoryLookupEntry(pagedirectory_t *directory, uint32_t virtual_addr) {
    if (directory) return &directory->entries[PAGEDIR_INDEX(virtual_addr)];
    else serialPrintf("vmm_directoryLookupEntry: Invalid page directory detected.\n");
    return 0;
}

// vmm_loadPDBR(uint32_t pdbr_addr) - Loads a new value into the PDBR address
void vmm_loadPDBR(uint32_t pdbr_addr) {
    asm ("mov %%eax, %0" :: "r"(pdbr_addr));
    asm ("mov %cr3, %eax");
}

// vmm_switchDirectory(pagedirectory_t *directory) - Changes the current page directory.
bool vmm_switchDirectory(pagedirectory_t *directory) {
    if (!directory) return false;
    currentDirectory = directory;
    vmm_loadPDBR(currentDirectory); // Load the new directory into the page directory base register (CR3)
    return true;
}

// vmm_flushTLBEntry(uint32_t addr) - Invalidate the current TLB entry
void vmm_flushTLBEntry(uint32_t addr) {
    asm volatile ("cli");
    asm volatile ("mov %%eax, %0" :: "r"(addr));
    asm volatile ("invlpg (%eax)");
    asm volatile ("sti");
}

// vmm_getCurrentDirectory() - Returns the current page directory
page_directory_t *vmm_getCurrentDirectory() {
    return currentDirectory;
}

// vmm_allocatePage(pte_t *entry) - Allocate VMM pages
bool vmm_allocatePage(pte_t *entry) {
    // Allocate a free physical frame
}

// vmm_mapPage(void *physical_addr, void *virtual_addr) - Maps a page from the physical address to its virtual address
void vmm_mapPage(void *physical_addr, void *virtual_addr) {

}