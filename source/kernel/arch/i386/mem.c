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


static pagedirectory_t *mem_kernelDirectory     = 0; // The kernel's page directory. Setup by mem_finalize
static pagedirectory_t *mem_currentDirectory    = 0; // Current page directory 
static uint32_t         mem_currentPDBR         = 0; // Current page directory base register address. This should be the same as pagedirectory_t.
char*                   mem_heapStart           = 0; // The start of our heap. Expanded using mem_sbrk
char*                   mem_kernelEnd           = 0; // Where the kernel ends and the heap begins.
char*                   mem_maxPMMUsage         = 0; // The maximum amount of bytes that the PMM can use (see mem_init())

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

    hal_disableHardwareInterrupts();
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
    asm volatile ("mov %0, %%cr3" :: "r"(addr));
    mem_currentPDBR = addr;
}


/**
 * @brief Remap a physical memory manager address to the identity mapped region
 * @param frame_address The address of the frame to remap
 */
uintptr_t mem_remapPhys(uintptr_t frame_address) {
    if ((uintptr_t)frame_address > IDENTITY_MAP_MAXSIZE) mem_outofmemory(0, "N/A. Maximum size of physical memory reached (512MB)");

    return ((uintptr_t)frame_address) | IDENTITY_MAP_REGION;
}



/**
 * @brief Switch the memory management directory
 * @param pagedir The page directory to switch to
 * 
 * @warning Pass something mapped by mem_clone() or something in the identity-mapped PMM region.
 *          Anything greater than IDENTITY_MAP_MAXSIZE will be truncated in the PDBR.
 * 
 * @returns -EINVAL on invalid, 0 on success.
 */
int mem_switchDirectory(pagedirectory_t *pagedir) {
    if (!pagedir) return -EINVAL;
    
    serialPrintf("mem: 0x%x - loading pdbr to 0x%x\n", pagedir, (uintptr_t)pagedir & ~IDENTITY_MAP_REGION);

    mem_load_pdbr((uintptr_t)pagedir & ~IDENTITY_MAP_REGION);

    // TODO: Should we use mem_remapPhys?
    mem_currentDirectory = pagedir;
    vmm_switchDirectory(pagedir);

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
    pagetable_t *table = (pagetable_t*)mem_remapPhys(MEM_VIRTUAL_TO_PHYS(pde));
    pte_t *page = &table->entries[MEM_PAGETBL_INDEX(virtaddr)];
    if (!page || !pte_ispresent(*page)) {
        serialPrintf("mem: pte not found at 0x%x\n", virtaddr);
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
    pde_t *pde = &(directory->entries[MEM_PAGEDIR_INDEX(virtaddr)]);
    if (!pde || !pde_ispresent(*pde)) {
        // The user might want us to create this directory.
        if (!(flags & MEM_CREATE)) goto bad_page;

        serialPrintf("mem: pde not present, creating a new one at vaddr 0x%x...\n", virtaddr);


        // Create a new table
        void *block = pmm_allocateBlock();
        if (!block) mem_outofmemory(1, "PDE allocation in get page");

        // Zero it
        memset((void*)mem_remapPhys((uintptr_t)block), 0, PAGE_SIZE);

        serialPrintf("mem: pde created at 0x%x\n", block);

        // Create a new entry
        pde_addattrib(pde, PDE_PRESENT);
        pde_addattrib(pde, PDE_WRITABLE);
        pde_addattrib(pde, PDE_USER);
        pde_setframe(pde, (uint32_t)block);
    }

    // Calculate the PTE index
    // Table allocation isn't necessary
    pagetable_t *table = (pagetable_t*)mem_remapPhys(MEM_VIRTUAL_TO_PHYS(pde));

    return &(table->entries[MEM_PAGETBL_INDEX(virtaddr)]);


bad_page:
    return NULL;
}


/**
 * @brief Map a physical address to a virtual address
 * 
 * @param dir The directory to map them in (leave blank for current)
 * @param phys The physical address
 * @param virt The virtual address
 */
void mem_mapAddress(pagedirectory_t *dir, uintptr_t phys, uintptr_t virt) {
    pagedirectory_t *directory = dir;
    if (dir == NULL) directory = mem_getCurrentDirectory();

    pte_t *page = mem_getPage(directory, virt, MEM_CREATE);
    pte_setframe(page, phys);
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
    if (flags & MEM_FREE_PAGE) {
        if (pte_getframe(*page)) pmm_freeBlock((void*)pte_getframe(*page));
        *page = 0;
        return;
    }

    // MEM_NOALLOC specifies we shouldn't allocate something for the page.
    if (!(flags & MEM_NOALLOC)) {
        void *block = pmm_allocateBlock();
        if (!block) mem_outofmemory(1, "page allocation");

        pte_setframe(page, (uint32_t)block);
    } else {
        serialPrintf("mem: NOALLOC specified (debug)\n");
    }

    

    // Let's setup the bits
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
    if (!page) return;
    mem_allocatePage(page, MEM_FREE_PAGE);
    
}

/**
 * @brief Get the current page directory
 */
pagedirectory_t *mem_getCurrentDirectory() {
    return mem_currentDirectory;
}

/**
 * @brief Get the kernel page directory
 */
pagedirectory_t *mem_getKernelDirectory() {
    return mem_kernelDirectory;
}


/**
 * @brief Internal function to handle usermode copies
 * 
 * @param dest The destination page directory
 * @param pd_idx The index (of the entry) within the page directory
 * @param src_pt The source page table
 * 
 * @warning This relies on use of sbrk()
 * @returns The block address of the table.
 */
void *mem_copyUserPT(pagedirectory_t *dest, uint32_t pd_idx, pagetable_t *src_pt) {
    // This function is useful because it allows usermode pages to be handled entirely by the kernel
    // We can employ CoW and update reference counts for tables, or whatever else should be done
    // For now, I'm lazy, let's just clone it normally

    // Allocate a new page table
    void *block = pmm_allocateBlock();
    if (!block) mem_outofmemory(1, "table allocation when cloning");

    pagetable_t *table = (pagetable_t*)mem_remapPhys((uintptr_t)block);

    for (int i = 0; i < 1024; i++) {
        if (!pte_getframe(src_pt->entries[i])) continue; // No frame address is even registered

        pte_t *src_page = &src_pt->entries[i];
        pte_t *dest_page = &table->entries[i];

        // Allocate a new frame to hold the data
        uintptr_t src_addr = (pd_idx << 22) | (i << 12);
        uintptr_t dest_addr = src_addr;

        // Setup its frame
        pte_setframe(dest_page, src_addr); 

        // Now that we're ready to go, grab a page from the memory system for our dest pd.
        // The page itself is discarded, it is simply created.
        pte_t *new_page = mem_getPage(dest, dest_addr, MEM_CREATE);
        if (new_page == NULL) panic("reduceOS", "memory", "Clone failed: Failed to create a page in destination page");
        mem_allocatePage(new_page, 0);

        // Ugly, but it works
        if (*src_page & PTE_PRESENT)        pte_addattrib(dest_page, PTE_PRESENT);
        if (*src_page & PTE_WRITABLE)       pte_addattrib(dest_page, PTE_WRITABLE);
        if (*src_page & PTE_USER)           pte_addattrib(dest_page, PTE_USER);
        if (*src_page & PTE_WRITETHROUGH)   pte_addattrib(dest_page, PTE_WRITETHROUGH);
        if (*src_page & PTE_NOT_CACHEABLE)  pte_addattrib(dest_page, PTE_NOT_CACHEABLE);
        if (*src_page & PTE_ACCESSED)       pte_addattrib(dest_page, PTE_ACCESSED);
        if (*src_page & PTE_DIRTY)          pte_addattrib(dest_page, PTE_DIRTY);

        // Create a temporary page. When we copy the contents, the data will be written to the frame, which is shared by both temp & dest pages
        // We can do this by mem_sbrking. TODO: THIS IS NOT A GOOD IDEA!
        void *temporary_vaddr = mem_sbrk(PAGE_SIZE);

        pte_t *temp_page = mem_getPage(NULL, (uintptr_t)temporary_vaddr, 0);
        uintptr_t orig_paddr = mem_getPhysicalAddress(NULL, (uintptr_t)temporary_vaddr);

        if (!temp_page) panic("reduceOS", "memory", "Clone failed: sbrk() did not succeed or failed to return a proper value");
        pte_setframe(temp_page, mem_getPhysicalAddress(dest, dest_addr));
        
        // Copy the data of the usermode page into this new page
        memcpy((void*)temporary_vaddr, (void*)src_addr, PAGE_SIZE);
        
        // Reset the old frame
        pte_setframe(temp_page, orig_paddr);

        // Undo sbrk(). We have to reset the old frame because it will be freed in the physical memory manager.
        mem_sbrk(-PAGE_SIZE);
    }

    return block;
}

/**
 * @brief Clone a page directory.
 * 
 * This is a full PROPER page directory clone. The old one just memcpyd the pagedir.
 * This function does it properly and clones the page directory, its tables, and their respective entries fully.
 * 
 * @param pd_in The source page directory. Keep as NULL to clone the current page directory.
 * @returns The page directory on success
 */
pagedirectory_t *mem_clone(pagedirectory_t *pd_in) {
    pagedirectory_t *source = pd_in;
    if (source == NULL) source = mem_getCurrentDirectory(); // NULL specified, use current directory
    
    void *pd_block = pmm_allocateBlock();
    if (!pd_block) mem_outofmemory(1, "page directory allocation");
    pagedirectory_t *pd_out = (pagedirectory_t*)mem_remapPhys((uintptr_t)pd_block);


    // Let's start cloning
    for (int pd = 0; pd < 1024; pd++) {
        pde_t *src_pde = &source->entries[pd];
        if (!pde_ispresent(*src_pde)) continue;

        // Construct a new table and add it to our output
        void *dest_table_block = pmm_allocateBlock();
        if (!dest_table_block) mem_outofmemory(1, "destination table allocation in clone");
        
        pagetable_t *dest_table = (pagetable_t*)mem_remapPhys((uintptr_t)dest_table_block);
        memset(dest_table, 0, sizeof(pagetable_t));

        // Construct a PDE
        pde_t *dest_pde = &pd_out->entries[pd];
        
        // TODO: We shouldn't do this!
        pde_addattrib(dest_pde, PDE_PRESENT);
        pde_addattrib(dest_pde, PDE_WRITABLE);
        pde_addattrib(dest_pde, PDE_USER);

        pde_setframe(dest_pde, (uint32_t)dest_table_block);

        pagetable_t *src_table = (pagetable_t*)mem_remapPhys(MEM_VIRTUAL_TO_PHYS(src_pde));
        
        // Now, time to copy the pages
        for (int page = 0; page < 1024; page++) {
            pte_t *src_page = &src_table->entries[page];
            if (pte_ispresent(*src_page)) {
                pte_t *dest_page = &dest_table->entries[page];

                if (*src_page & PTE_USER) {
                    // Usermode page. Bah humbug!
                }

                // Copy it because whatever
                *dest_page = *src_page;
            } 
        }
        

    }



    return pd_out;
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
    extern uint32_t *frames;
    mem_heapStart += nframes;

    // Now we should copy the multiboot info
    mem_copyMultiboot();
    

    // Add an extra page to align it correctly, else we'll end up in bad situations.
    mem_heapStart = (char*)((uintptr_t)(mem_heapStart + 0x1000) & ~0xFFF); 

    // Deinitialize heap
    extern uint32_t text_start; // This is the start of the kernel's page
    pmm_deinitRegion((uint32_t)&text_start, (uint32_t)mem_heapStart - (uint32_t)&text_start);

    // HACKS!
    pmm_deinitRegion(0xA0000,   0xB0000);     // VGA video memory (in some cases), still used by QEMU
    pmm_deinitRegion(0x250000,  0x10000);
    pmm_deinitRegion(0x320000,  0x40000);
    pmm_deinitRegion(0x2e0000,  0x0C000);


    // Get a page directory
    pagedirectory_t *dir = (pagedirectory_t*)pmm_allocateBlocks(6); // why 6
    memset(dir, 0x0, sizeof(pagedirectory_t));



    // We only have access to 4GB of VAS because of 32-bit protected mode (and lack of PAE support)
    // The physical memory manager is very much needed after paging is enabled, specifically for PFA
    // It's best to map this code to a specific range.
    // reduceOS can map a maximum of 512 MB of addressible memory. This is specified in mem.h
    // Calculate how many pages the PMM is using
    size_t frame_bytes = nframes * 4096; 
    frame_bytes = (frame_bytes + 0xFFF) & ~0xFFF;

    if (frame_bytes > IDENTITY_MAP_MAXSIZE) {
        serialPrintf("mem: WARNING! Too much memory for identity maps (0x%x bytes available, maximum identity map is 0x%x)!\n", frame_bytes, IDENTITY_MAP_MAXSIZE);   
        
        // Truncate to max size
        frame_bytes = IDENTITY_MAP_MAXSIZE;
    }

    size_t frame_pages = frame_bytes >> 12;

    mem_maxPMMUsage = (char*)frame_bytes;

    // Identity mapping time! 

    uintptr_t frame = 0x0; // The frame currently being done by the loop
    uintptr_t table_frame = 0x0; // The frame that the table should be put in
    int loop_cycles = (frame_pages + 1024) / 1024;

    // We'll use a variable to keep track of how many pages have been actually mapped
    int pages_mapped = 0;

    for (int i = 0; i < loop_cycles; i++) {
        pagetable_t *table = (pagetable_t*)pmm_allocateBlock(); 
        memset(table, 0x0, sizeof(pagetable_t));


        for (int page = 0; page < 1024; page++) {
            // This seems like a bad way to map..
            pte_t page = 0;

            pte_addattrib(&page, PTE_PRESENT);
            pte_addattrib(&page, PTE_WRITABLE);
            pte_addattrib(&page, PTE_USER);
            pte_setframe(&page, frame);

            table->entries[MEM_PAGETBL_INDEX(frame + IDENTITY_MAP_REGION)] = page;

            pages_mapped++;
            if (pages_mapped == (int)frame_pages) break;

            frame += 0x1000;
        }


        // Now create a PDE
        pde_t *entry = &dir->entries[MEM_PAGEDIR_INDEX(table_frame + IDENTITY_MAP_REGION)];
        pde_addattrib(entry, PDE_PRESENT);
        pde_addattrib(entry, PDE_WRITABLE);
        pde_addattrib(entry, PDE_USER);
        pde_setframe(entry, (uint32_t)table);
        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)frame_pages) break;
    }



    // We also need to map the kernel and the extra stuff we tossed in.
    // This is a little complicated, but it's not too hard.

    // Calculate how many pages we need to allocate.
    uintptr_t heap_start_aligned = ((uintptr_t)mem_heapStart + 0xFFF) & 0xFFFFF000; 
    uintptr_t kern_pages = (uintptr_t)heap_start_aligned >> PAGE_SHIFT; // Shifting right 0x1000

    // Let's map. We'll calculate the amount of cycles to run the initial loop
    loop_cycles = (kern_pages + 1024) / 1024;
    pages_mapped = 0;

    // Reset values
    frame = 0x0;
    table_frame = 0x0;
    for (int i = 0; i < loop_cycles; i++) {
        pagetable_t *table = (pagetable_t*)pmm_allocateBlock(); 
        memset(table, 0x0, sizeof(pagetable_t));


        for (int page = 0; page < 1024; page++) {
            // This seems like a bad way to map..
            pte_t page = 0;

            pte_addattrib(&page, PTE_PRESENT);
            pte_addattrib(&page, PTE_WRITABLE);
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
        pde_setframe(entry, (uint32_t)table);
        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)kern_pages) break;
    }
    

    // Setup final variables
    mem_kernelEnd = mem_heapStart;
    mem_kernelDirectory = dir;

    // Final prep work
    isrRegisterInterruptHandler(14, (ISR)pageFault);
    mem_switchDirectory(mem_kernelDirectory);
    vmm_enablePaging();

    serialPrintf("mem: The memory allocation system has initialized. Statistics:\n");
    serialPrintf("\tHeap initialized to 0x%x, and addresses 0x%x - 0x%x were mapped\n", mem_heapStart, &text_start, heap_start_aligned);
    serialPrintf("\tAvailable physical memory: %i KB\n", globalInfo->m_memoryHi - globalInfo->m_memoryLo);
    serialPrintf("\tHas crashed yet: not yet\n");
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
        // just reverse everything lol
        for (char *i = mem_heapStart; i > mem_heapStart + b; i -= 0x1000) {
            
            mem_freePage(mem_getPage(NULL, (uintptr_t)i, 0));
        }


        char *oldStart = mem_heapStart;
        mem_heapStart += b;

        serialPrintf("mem: sbrk shrunk heap from 0x%x to 0x%x (b was %i)\n", oldStart, mem_heapStart, b);
        return oldStart;
    }

    // Let's allocate some pages
    for (char *i = mem_heapStart; i < mem_heapStart + b; i += 0x1000) {
        pte_t *existing_page = (mem_getPage(NULL, (uintptr_t)i, 0));
        if (existing_page && pte_ispresent(*existing_page)) {
            serialPrintf("mem: WARNING! Expanding into unknown memory region at 0x%x!\n", i);

            // hey look, free memory!
            continue;
        }

        pte_t *page = mem_getPage(NULL, (uintptr_t)i, MEM_CREATE);
        mem_allocatePage(page, MEM_WRITETHROUGH);
    }


    char *oldStart = mem_heapStart;
    mem_heapStart += b;

    serialPrintf("mem: Successfully allocated 0x%x - 0x%x with a b request of 0x%x\n", oldStart, mem_heapStart, b);

    return oldStart;
}


/**
 * @brief Finalize any changes to the memory system
 */
void mem_finalize() {
    // Fill in anything you need here.

    serialPrintf("mem: Finalized memory system successfully.\n");
}
