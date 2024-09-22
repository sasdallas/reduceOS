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
#include <libk_reduced/errno.h>



static pagedirectory_t *mem_currentDirectory    = 0; // Current page directory 
static uint32_t         mem_currentPDBR         = 0; // Current page directory base register address. This should be the same as pagedirectory_t.
static char*            mem_heapStart           = 0; // The start of our heap. Expanded using mem_sbrk


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
    mem_currentPDBR = addr;
}


/**
 * @brief Switch the memory management directory
 * @param pagedir The page directory to switch to
 * @note This will also reload the PDBR.
 * @returns -EINVAL on invalid, 0 on success.
 */
int mem_switchDirectory(pagedirectory_t *pagedir) {
    if (!pagedir) return -EINVAL;

    mem_load_pdbr((uintptr_t)pagedir);
    mem_currentDirectory = pagedir;
    return 0; 
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
    
    // Calculate the PDE index
    pde_t *pde = &directory->entries[MEM_PAGEDIR_INDEX(virtaddr)];
    if (!pde || !pde_ispresent(*pde)) {
        serialPrintf("mem: pde not found at 0x%x\n", pde);
        return NULL;
    }

    // Calculate the PTE index
    pagetable_t *table = (pagetable_t*)MEM_VIRTUAL_TO_PHYS(pde);
    pte_t *page = &table->entries[MEM_PAGETBL_INDEX(virtaddr)];
    if (!page || !pte_ispresent(*page)) {
        serialPrintf("mem: pte not found at 0x%x\n", page);
        return NULL;
    }
    
    // Can't just return *page, we need to get the page offset and append it.
    return (uintptr_t)(pte_getframe(*page) + (virtaddr & 0xFFF));
}


/**
 * @brief Returns the page entry requested as a PTE
 * @param dir The directory to search. Specify NULL for current directory
 * @param addr The virtual address of the page
 * @param flags The flags of the page to look for
 * 
 * @warning Specifying MEM_CREATE will only create needed structures, it will NOT allocate the page!
 *          Please use a function such as mem_allocatePage to do that.
 */
pte_t *mem_getPage(pagedirectory_t *dir, uintptr_t addr, uintptr_t flags) {
    // TODO: This function could be merged with mem_getPhysicalAddress

    pagedirectory_t *directory = dir;
    if (directory == NULL) {
        directory = mem_currentDirectory;
    }

    // Page addresses are divided into 3 parts:
    // - The index of the PDE (bits 22-31)
    // - The index of the PTE (bits 12-21)
    // - The page offset (bits 0-11)

    uintptr_t virtaddr = addr; // You can do any modifications needed to addr

    // Calculate the PDE index
    pde_t *pde = &directory->entries[MEM_PAGEDIR_INDEX(virtaddr)];
    if (!pde || !pde_ispresent(*pde)) {
        serialPrintf("mem: pde not present, creating a new one at vaddr 0x%x...\n", virtaddr);

        // The user might want us to create this directory.
        if (!(flags & MEM_CREATE)) goto bad_page;

        

        // Create a new table
        pagetable_t *table = (pagetable_t*)pmm_allocateBlock();
        if (!table) mem_outofmemory(1, "PDE allocation in get page");

        memset(table, 0, sizeof(pagetable_t));

        // Create a new entry
        pde_t *e = &directory->entries[MEM_PAGEDIR_INDEX(virtaddr)];

        pde_addattrib(e, PDE_PRESENT);
        pde_addattrib(e, PDE_WRITABLE);
        pde_addattrib(e, PDE_USER);
        pde_setframe(e, (uint32_t)table);
    }

    // Calculate the PTE index
    // Table allocation isn't necessary
    pagetable_t *table = (pagetable_t*)MEM_VIRTUAL_TO_PHYS(pde);

    return &table->entries[MEM_PAGETBL_INDEX(virtaddr)];


    
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
        void *block = pmm_allocateBlock();
        

        if (!block) mem_outofmemory(1, "page allocation");

        pte_setframe(page, (uint32_t)block);
    } else {
        serialPrintf("mem: NOALLOC specified (debug)\n");
    }


    // Let's setup the bits
    //pte_addattrib(page, PTE_PRESENT);
    //pte_addattrib(page, PTE_WRITABLE);
    *page = (flags & MEM_NOT_PRESENT) ? (*page & ~PTE_PRESENT) : (*page | PTE_PRESENT);
    *page = (flags & MEM_KERNEL) ? (*page & ~PTE_USER) : (*page | PTE_USER);
    *page = (flags & MEM_READONLY) ? (*page & ~PTE_WRITABLE) : (*page | PTE_WRITABLE);
    *page = (flags & MEM_WRITETHROUGH) ? (*page & ~PTE_WRITETHROUGH) : (*page | PTE_WRITETHROUGH);
    *page = (flags & MEM_NOT_CACHEABLE) ? (*page & ~PTE_NOT_CACHEABLE) : (*page | PTE_NOT_CACHEABLE);
}


/**
 * @brief Free a page
 * 
 * @param page The page to free 
 */
void mem_freePage(pte_t *page) {
    // Get the physical address and free it
    uintptr_t paddr = pte_getframe(*page);
    if (paddr) pmm_freeBlock((void*)paddr);
    mem_allocatePage(0, MEM_NOT_PRESENT | MEM_NOALLOC);
}

/**
 * @brief Get the current page directory
 */
pagedirectory_t *mem_getCurrentDirectory() {
    return mem_currentDirectory;
}


/**
 * @brief Clone a page directory.
 * 
 * This is a full PROPER page directory clone. The old one just memcpyd the pagedir.
 * That's like 50% fine and 50% completely boinked (or 100%, honestly none of this vmm should work).
 * This function does it properly and clones the page directory, its tables, and their respective entries fully.
 * 
 * @param pd_in The source page directory. Keep as NULL to clone the current page directory.
 * @param pd_out The output page directory.
 * @returns 0 on success, anything else is failure (usually an errno)
 */
int mem_clone(pagedirectory_t *pd_in, pagedirectory_t *pd_out) {
    pagedirectory_t *source = pd_in;
    if (source == NULL) source = mem_getCurrentDirectory(); // NULL specified, use current directory
    if (pd_out == NULL) return -EINVAL; // stupid users

    // stub
    return 0;
}

extern multiboot_info *globalInfo;

/**
 * @brief Internal function to copy the multiboot information to the heap
 */
static void mem_copyMultiboot() {
    multiboot_info *info_orig = &(*globalInfo);
    globalInfo = (multiboot_info*)mem_heapStart;
    memcpy(globalInfo, info_orig, sizeof(multiboot_info));
    mem_heapStart += sizeof(multiboot_info);

    // Another hack to copy over multiboot module INFORMATION
    // Don't forget to update mods address!
    uint32_t oldAddr = globalInfo->m_modsAddr;
    globalInfo->m_modsAddr = (uint32_t)mem_heapStart;

    int i;
    multiboot_mod_t *mod;

    for (i = 0, mod = (multiboot_mod_t*)oldAddr; i < (int)globalInfo->m_modsCount; i++, mod++) {
        multiboot_mod_t *clone = (multiboot_mod_t*)mem_heapStart; // The new module will be located at the heap start
        memcpy(clone, mod, sizeof(multiboot_mod_t)); // Clone the module
        mem_heapStart += sizeof(multiboot_mod_t);

        strcpy(mem_heapStart, (char*)clone->cmdline);
        clone->cmdline = (uint32_t)mem_heapStart;
        
        mem_heapStart += strlen((char*)clone->cmdline);

        mod = clone;

        // Now we need to clone the content
        size_t modSize = clone->mod_end - clone->mod_start;
        memcpy((void*)mem_heapStart, (void*)clone->mod_start, modSize);
        clone->mod_start = (uint32_t)mem_heapStart;
        clone->mod_end = (uint32_t)mem_heapStart + modSize;
        mem_heapStart += modSize;


        serialPrintf("kernel: Multiboot module at 0x%x - 0x%x (%s)\n", clone->mod_start, clone->mod_end, clone->cmdline);
    }

    // Let's copy over the command line.
    char *cmdline = mem_heapStart;
    strcpy(cmdline, (char*)globalInfo->m_cmdLine);
    globalInfo->m_cmdLine = (uint32_t)cmdline;
    mem_heapStart += strlen(cmdline);

    // Now, the memory map
    oldAddr = globalInfo->m_mmap_addr;
    globalInfo->m_mmap_addr = (uint32_t)mem_heapStart;
    for (uint32_t i = 0; i < oldAddr; i += sizeof(memoryRegion_t)) {
        memoryRegion_t *region = (memoryRegion_t*)(oldAddr + i);
        memoryRegion_t *destRegion = (memoryRegion_t*)mem_heapStart;
        memcpy((void*)destRegion, (void*)region, sizeof(memoryRegion_t));
        mem_heapStart += sizeof(memoryRegion_t);
    }

    
}

/**
 * @brief Initialize the memory management subsystem
 * 
 * This function will identity map the kernel into memory and setup page tables.
 */
void mem_init() {
    // Calculate the kernel's ending pointer
    uintptr_t endPtr = ((uintptr_t)&end + 0xFFF) & 0xFFFFF000;

    // Setup the heap
    mem_heapStart = (char*)endPtr;

    // Initialize PMM
    pmmInit((globalInfo->m_memoryHi - globalInfo->m_memoryLo), mem_heapStart);

    // Use the memory map from multiboot to setup pmm regions
    pmm_initializeMemoryMap(globalInfo);

    // We cannot use PMM to allocate because we need to first throw stuff on the heap, then deinitialize the heap.

    // Update mem_heapStart
    extern uint32_t nframes;
    mem_heapStart += nframes;

    // Now we should copy the multiboot info
    mem_copyMultiboot();
    

    // Add an extra page to align it correctly, else we'll end up in bad situations.
    mem_heapStart = (char*)((uintptr_t)(mem_heapStart + 0x1000) & ~0xFFF); 

    // Deinitialize heap
    extern uint32_t text_start; // This is the start of the kernel's page
    pmm_deinitRegion((uintptr_t)&text_start, (uint32_t)mem_heapStart - (uint32_t)&text_start);

    // Now, it's identity mapping time. We need to map the kernel and the extra stuff we tossed in.
    // This is a little complicated, but it's not too hard.

    // Get a page directory
    pagedirectory_t *dir = pmm_allocateBlocks(6);
    memset(dir, 0x0, PAGE_SIZE * 6);
    

    // Calculate how many pages we need to allocate.
    uintptr_t heap_start_aligned = ((uintptr_t)mem_heapStart + 0xFFF) & 0xFFFFF000; 
    uintptr_t kern_pages = (uintptr_t)heap_start_aligned >> PAGE_SHIFT; // Shifting right 0x1000

    // Let's map. We'll calculate the amount of cycles to run the initial loop
    int initial_loop_cycles = (kern_pages + 1024) / 1024;
    
    // We'll use a variable to keep track of how many pages have been actually mapped
    int pages_mapped = 0;

    uintptr_t frame = 0x0; // The frame currently being done by the loop
    uintptr_t table_frame = 0x0; // The frame that the table should be put in

    for (int i = 0; i < initial_loop_cycles; i++) {
        pagetable_t *table = (pagetable_t*)pmm_allocateBlock();
        memset(table, 0x0, sizeof(pagetable_t));

        for (int page = 0; page < 1024; page++) {
            // This seems like a bad way to map..
            pte_t page = 0;

            pte_addattrib(&page, PTE_PRESENT);
            pte_addattrib(&page, PTE_WRITABLE);
            pte_addattrib(&page, PTE_USER);
            pte_setframe(&page, frame);

            table->entries[MEM_PAGETBL_INDEX(frame)] = page;

            pages_mapped++;
            if (pages_mapped == (int)kern_pages) break;

            frame += 0x1000;
        }


        // Now create a PDE
        pde_t *entry = &dir->entries[MEM_PAGEDIR_INDEX(table_frame)];
        pde_addattrib(entry, PDE_PRESENT);
        pde_addattrib(entry, PDE_WRITABLE);
        pde_addattrib(entry, PDE_USER);
        pde_setframe(entry, (uint32_t)table);
        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)kern_pages) break;
    }
    

    // Final prep work
    isrRegisterInterruptHandler(14, (ISR)pageFault);
    mem_switchDirectory(dir);
    vmm_switchDirectory(dir);
    vmm_enablePaging();
}

/**
 * @brief Expand/shrink the kernel heap
 * 
 * @param b The amount of bytes to allocate, needs to a multiple of PAGE_SIZE
 * @returns Address of the start of the bytes 
 */
void *mem_sbrk(int b) {
    if (!mem_heapStart) {
        // bro how
        panic("reduceOS", "mem_sbrk", "Heap not yet ready");
    }

    if (!b) return mem_heapStart;


    if (b & 0xFFF) {
        panic("reduceOS", "mem_sbrk", "Size passed is not a multiple of 4096");
    }


    // Do they want to shrink the kernel heap?
    if (b < 0) {
        /*
        // just reverse everything lol
        for (char *i = mem_heapStart; i > mem_heapStart - b; i -= 0x1000) {
            pte_t *page = mem_getPage(NULL, (uintptr_t)i, 0);
            if (page == NULL) panic("reduceOS", "sys_sbrk", "Page not present");

            mem_freePage(page);
        }


        char *oldStart = mem_heapStart;
        mem_heapStart += b;

        serialPrintf("mem: sbrk shrunk heap from 0x%x to 0x%x (b was %i)\n", oldStart, mem_heapStart, b);
        return oldStart;
        */

       return mem_heapStart;
    }

    // Let's allocate some pages
    for (char *i = mem_heapStart; i < mem_heapStart + b; i += 0x1000) {
        if (pte_ispresent(*(mem_getPage(NULL, (uintptr_t)i, 0)))) {
            serialPrintf("mem: WARNING! Expanding into unknown memory region at 0x%x!\n", i);

            // hey look, free memory!
            continue;
        }

        pte_t *page = mem_getPage(NULL, (uintptr_t)i, MEM_CREATE);
        mem_allocatePage(page, MEM_WRITETHROUGH | MEM_KERNEL);
    }


    char *oldStart = mem_heapStart;
    mem_heapStart += b;

    serialPrintf("mem: Successfully allocated 0x%x - 0x%x with a b request of 0x%x\n", oldStart, mem_heapStart, b);
    return oldStart;
}
