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

// General includes
#include <kernel/mem.h>
#include <kernel/pmm.h>

// Arch-specific
#include <kernel/arch/i386/page.h>

// These need to be architecture-specific
#include <kernel/vmm_pte.h>
#include <kernel/vmm_pde.h>

// Libk includes
#include <libk_reduced/stdio.h>

#pragma GCC diagnostic ignored "-Wunused-variable"

static pagedirectory_t *mem_currentDirectory    = 0; // Current page directory 
static uint32_t         mem_currentPDBR         = 0; // Current page directory base register address. This should be the same as pagedirectory_t.


extern uint32_t end;

/**
 * @brief Die in the cold winter
 * @param pages How many pages were trying to be allocated
 * @param seq The sequence of failure
 */
void mem_outofmemory(int pages, char *seq) {
    panic_prepare();
    serialPrintf("*** The memory manager could not successfully allocate enough memory.\n");
    serialPrintf("*** Failed to allocate %i pages during %s\n", pages, seq);

    printf("*** The system has run out of memory.\n");
    printf("\nThis error indicates that your system has fully run out of memory and can no longer continue its operation.\n");
    printf("Please either do not open many resource intensive applications, or potentially use a larger pagefile\n"); // Good luck with that 
    printf("An application or OS bug may have also caused this. If you feel it is necessary, file a GitHub bug report (please do).\n");
    printf("\nFor more information, contact your system administrator.\n");

    panic_dumpStack(NULL);
    printf("\n");

    disableHardwareInterrupts();
    asm volatile ("hlt");
    for (;;);
    __builtin_unreachable();
}

/**
 * @brief Invalidate a page in the TLB
 * @param addr The address of the page 
 * @warning This function is only to be used when removing P-V mappings. Just free the page if it's identity.
 */
static inline void mem_invalidatePage(uintptr_t addr) {
    asm volatile ("invlpg (%0)" :: "r"(addr) : "memory");
    // TODO: When SMP is done, a TLB shootdown will happen here (slow)
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
    pde_t *pde = &dir->entries[MEM_PAGEDIR_INDEX(virtaddr)];
    if (!pde || !pde_ispresent(*pde)) return NULL;
    

    // Calculate the PTE index
    pagetable_t *table = (pagetable_t*)MEM_VIRTUAL_TO_PHYS(pde);
    pte_t *page = &table->entries[MEM_PAGETBL_INDEX(virtaddr)];
    if (!page || !pte_ispresent(*page)) return NULL;
    
    
    // Can't just return *page, we need to get the page offset and append it.
    return (uintptr_t)(pte_getframe(*page) + (virtaddr & 0xFFF));
}


/**
 * @brief Returns the page entry requested as a PTE
 * @param dir The directory to search. Specify NULL for current directory
 * @param virtaddr The virtual address of the page
 * @param flags The flags of the page to look for
 * 
 * @warning Specifying MEM_CREATE will only create needed structures, it will NOT allocate the page!
 *          Please use a function such as mem_allocatePage to do that.
 */
pte_t mem_getPage(pagedirectory_t *dir, uintptr_t virtaddr, uintptr_t flags) {
    // TODO: This function could be merged with mem_getPhysicalAddress

    pagedirectory_t *directory = dir;
    if (directory == NULL) directory = mem_currentDirectory;

    // Page addresses are divided into 3 parts:
    // - The index of the PDE (bits 22-31)
    // - The index of the PTE (bits 12-21)
    // - The page offset (bits 0-11)

    uintptr_t addr = virtaddr; // You can do any modifications needed to addr

    // Calculate the PDE index
    pde_t *pde = &dir->entries[MEM_PAGEDIR_INDEX(virtaddr)];
    if (!pde || !pde_ispresent(*pde)) {
        // The user might want us to create this directory.
        if (!(flags & MEM_CREATE)) goto bad_page;

        // Create a new table
        pagetable_t *table = (pagetable_t*)pmm_allocateBlock();
        if (!table) mem_outofmemory(1, "PDE allocation in get page");

        memset(table, 0, sizeof(pagetable_t));

        // Create a new entry
        pte_t *e = &directory->entries[MEM_PAGEDIR_INDEX(virtaddr)];

        // Map it in the table
        // TODO: Do we need to check flags?
        pde_addattrib(e, PDE_PRESENT);
        pde_addattrib(e, PDE_WRITABLE);
        pde_addattrib(e, PDE_USER);
        pde_setframe(e, (uint32_t)table);
    }

    // Calculate the PTE index
    // Table allocation isn't necessary
    pagetable_t *table = (pagetable_t*)MEM_VIRTUAL_TO_PHYS(pde);
    pte_t *page = &table->entries[MEM_PAGETBL_INDEX(virtaddr)];


    return *page;
    
bad_page:
    serialPrintf("[i386] mem: page at 0x%x not found with flags 0x%x\n", virtaddr, flags);
    return NULL;
}

/**
 * @brief Allocate a page using the physical memory manager
 * 
 * @param page The page object to use. Can be obtained with mem_getPage
 * @param flags The flags to follow when setting up the page
 * 
 * @note You can also use this function to set bits of a specific page - just specify @c MEM_NOALLOC in @p flags.
 * @warning The function will automatically allocate a PMM block if NOALLOC isn't specified.
 */
void mem_allocatePage(pte_t *page, uint32_t flags) {
    // MEM_NOALLOC specifies we shouldn't allocate something for the page.
    if (!(flags & MEM_NOALLOC)) {
        void *block = pmm_allocateBlock();  // TODO: THIS IS PROBABLY A BUG. WE ARENT DOING THIS CORRECTLY.
                                            // The best way of doing this is to find the first free frame and set the bit << by PAGE_SHIFT

        if (!block) mem_outofmemory(1, "page allocation");

        pte_setframe(page, (uint32_t)block);
    }


    // Let's setup the bits
    pte_addattrib(page, PTE_PRESENT);
    *page = (flags & MEM_KERNEL) ? (*page & ~PTE_USER) : (*page | PTE_USER);
    *page = (flags & MEM_READONLY) ? (*page & ~PTE_WRITABLE) : (*page | PTE_WRITABLE);
    *page = (flags & MEM_WRITETHROUGH) ? (*page & ~PTE_WRITETHROUGH) : (*page | PTE_WRITETHROUGH);
    *page = (flags & MEM_NOT_CACHEABLE) ? (*page & ~PTE_NOT_CACHEABLE) : (*page | PTE_NOT_CACHEABLE);
}


