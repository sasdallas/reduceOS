/**
 * @file source/kernel/arch/i386/mem.c
 * @brief i386 virtual memory manager
 * 
 * ! This file is in prototype !
 * This is the memory management facilities for the i386 architecture.
 * 
 * @see pmm.c for the physical memory manager - it is architecture-independent.
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/mem.h>
#include <kernel/pmm.h>

#include <kernel/arch/i386/page.h>

// These need to be architecture-independentized
#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>

#pragma GCC diagnostic ignored "-Wunused-variable"

static pagedirectory_t *mem_currentDirectory    = 0; // Current page directory 
static uint32_t         mem_currentPDBR         = 0; // Current page directory base register address. This should be the same as pagedirectory_t.


/**
 * @brief Invalidate a page in the TLB
 * @param addr The address of the page 
 * @warning This function is only to be used when removing P-V mappings. Just free the page if it's identity.
 */
static inline void mem_invalidatePage(uintptr_t addr) {
    asm volatile ("invlpg (%0)" :: "r"(addr) : "memory");
} 

/**
 * @brief Load new value into the PDBR
 * @param pagedir The address of the page directory to load
 */
static inline void mem_load_pdbr(uintptr_t addr) {
    asm volatile ("mov %%cr3, %0" :: "r"(addr));
}


/**
 * @brief Get the physical address of a virtual address
 * @param dir Can be NULL to use the current directory.
 * @param virtaddr The virtual address
 */
uintptr_t mem_getPhysicalAddress(pagedirectory_t *dir, uintptr_t virtaddr) {
    pagedirectory_t *directory = dir;
    if (directory == NULL) directory = mem_currentDirectory;

    // Page addresses are divided into 3 parts:
    // - The index of the PDE (bits 22-31)
    // - The index of the PTE (bits 12-21)
    // - The page offset (bits 0-11)

    uintptr_t addr = virtaddr; // You can do any modifications needed to addr

    // Calculate the PDE index
    pde_t *pde = &dir->entries[PAGEDIR_INDEX(virtaddr)];
    if (!pde || !pde_ispresent(*pde)) return NULL;
    

    // Calculate the PTE index
    pagetable_t *table = (pagetable_t*)VIRTUAL_TO_PHYS(pde);
    pte_t *page = &table->entries[PAGETBL_INDEX(virtaddr)];
    if (!page || !pte_ispresent(*page)) return NULL;
    
    
    // Can't just return *page, we need to get the page offset and append it.
    return (uintptr_t)(pte_getframe(*page) + (virtaddr & 0xFFF));
}
