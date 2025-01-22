/**
 * @file hexahedron/arch/x86_64/mem.c
 * @brief Memory management functions for x86_64
 * 
 * @warning A lot of functions in this file do not conform to the "standard" of unmapping
 *          physical addresses after you have finished. This is fine for now, but may have issues
 *          later.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/x86_64/mem.h>
#include <kernel/arch/x86_64/cpu.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/pmm.h>
#include <kernel/processor_data.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/misc/spinlock.h>

#include <stdint.h>
#include <string.h>

// Heap/MMIO/driver space
uintptr_t mem_kernelHeap                = 0xAAAAAAAAAAAAAAAA;   // Kernel heap
uintptr_t mem_driverRegion              = MEM_DRIVER_REGION;    // Driver space
uintptr_t mem_dmaRegion                 = MEM_DMA_REGION;       // DMA region

// Reference counts
uint8_t  *mem_pageReferences            = NULL;

// Spinlocks
static spinlock_t heap_lock = { 0 };
static spinlock_t driver_lock = { 0 };
static spinlock_t dma_lock = { 0 };

// Stub variables (for debugger)
uintptr_t mem_mapPool                   = 0xAAAAAAAAAAAAAAAA;
uintptr_t mem_identityMapCacheSize      = 0xAAAAAAAAAAAAAAAA;   

// Whether to use 5-level paging (TODO)
static int mem_use5LevelPaging = 0;

// Base page layout - loader uses this
page_t mem_kernelPML[3][512] __attribute__((aligned(PAGE_SIZE))) = {0};

// Low base PDPT/PD/PT (identity mapping space for kernel/other stuff)
page_t mem_lowBasePDPT[512] __attribute__((aligned(PAGE_SIZE))) = {0}; 
page_t mem_lowBasePD[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_lowBasePT[512*3] __attribute__((aligned(PAGE_SIZE))) = {0};

// High base PDPT/PD/PT (identity mapping space for anything)
// NOTE: This is not my implementation of high base mapping (see ToaruOS)
page_t mem_highBasePDPT[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_highBasePDs[64][512] __attribute__((aligned(PAGE_SIZE))) = {0}; // If we're already using 2MiB paging, why bother with PTs? 

// Heap PDPT/PD/PT
page_t mem_heapBasePDPT[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_heapBasePD[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_heapBasePT[512*3] __attribute__((aligned(PAGE_SIZE))) = {0};


/**
 * @brief Get the current directory (just current CPU)
 */
page_t *mem_getCurrentDirectory() {
    return current_cpu->current_dir;
}

/**
 * @brief Get the kernel page directory/root-level PML
 */
page_t *mem_getKernelDirectory() {
    return (page_t*)mem_remapPhys((uintptr_t)&mem_kernelPML[0], 0);
}

/**
 * @brief Get the current position of the kernel heap
 * @returns The current position of the kernel heap
 */
uintptr_t mem_getKernelHeap() {
    return mem_kernelHeap;
}


/**
 * @brief Increment a page refcount
 * @param page The page to increment reference counts of
 * @returns The number of reference counts or 0 if maximum is reached
 */
int mem_incrementPageReference(page_t *page) {
    if (!page) return 0; // ???
    if (!page->bits.present) {
        dprintf(ERR, "Tried incrementing reference count on non-present page\n");
        return 0;
    }

    // First get the index in refcounts of the frame
    uintptr_t idx = page->bits.address;
    if (mem_pageReferences[idx] == UINT8_MAX) {
        // We're too high, return 0 and hope they make a copy of the page
        return 0; 
    }

    mem_pageReferences[idx]++;
    return mem_pageReferences[idx];
}

/**
 * @brief Decrement a page refcount
 * @param page The page to decrement the reference count of
 * @returns The number of reference counts. Panicks if 0
 */
int mem_decrementPageReference(page_t *page) {
    if (!page) return 0; // ???
    if (!page->bits.present) {
        dprintf(ERR, "Tried decrementing reference count on non-present page\n");
        return 0;
    }

    // First get the index in refcounts of the frame
    uintptr_t idx = page->bits.address;
    if (mem_pageReferences[idx] == 0) {
        // Bail out!
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "pageref", "*** Tried to release reference on page with 0 references (bug)\n");
        __builtin_unreachable();
    }

    mem_pageReferences[idx]--;
    return mem_pageReferences[idx];
}

/**
 * @brief Switch the memory management directory
 * @param pagedir The page directory to switch to, or NULL for the kernel region
 * 
 * @warning Pass something mapped by mem_clone() or something in the identity-mapped PMM region.
 *          Anything greater than IDENTITY_MAP_MAXSIZE will be truncated in the PDBR.
 * 
 * @returns -EINVAL on invalid, 0 on success.
 */
int mem_switchDirectory(page_t *pagedir) {
    if (!pagedir) pagedir = (page_t*)mem_remapPhys((uintptr_t)mem_getKernelDirectory(), 0); // remapping?
    current_cpu->current_dir = pagedir;

    // Load PDBR
    asm volatile ("movq %0, %%cr3" :: "r"((uintptr_t)pagedir & ~0xFFF));
    return 0;

}

/**
 * @brief Create a new, completely blank virtual address space
 * @returns A pointer to the VAS
 */
page_t *mem_createVAS() {
    page_t *vas = (page_t*)mem_remapPhys(pmm_allocateBlock(), PMM_BLOCK_SIZE);
    memset((void*)vas, 0, PMM_BLOCK_SIZE);
    return vas;
}

// TODO: Destroy VAS function?

/**
 * @brief Clone a page directory.
 * 
 * This is a full PROPER page directory clone.
 * This function does it properly and clones the page directory, its tables, and their respective entries fully.
 * It also has the option to do CoW on usermode pages
 * 
 * @ref https://github.com/klange/toaruos/blob/master/kernel/arch/x86_64/mmu.c
 * 
 * 
 * @param dir The source page directory. Keep as NULL to clone the current page directory.
 * @returns The page directory on success
 */
page_t *mem_clone(page_t *dir) {
    if (!dir) dir = mem_getCurrentDirectory();

    // Create a new VAS
    page_t *dest = mem_createVAS();

    // Copy top half
    memcpy(&dest[256], &dir[256], 256 * sizeof(page_t));

    // Copy PDPTs 
    for (size_t pdpt = 0; pdpt < 256; pdpt++) {
        page_t *pdpt_srcentry = &dir[pdpt]; // terrible naming lol
        if (!(pdpt_srcentry->bits.present)) continue;

        page_t *pdpt_destentry = &dest[pdpt];

        // Create a new PDPT
        uintptr_t pdpt_dest_block = pmm_allocateBlock();
        page_t *pdpt_dest = (page_t*)mem_remapPhys(pdpt_dest_block, PAGE_SIZE);
        memset((void*)pdpt_dest, 0, PAGE_SIZE); 

        // Do a raw copy but set the frame
        pdpt_destentry->data = pdpt_srcentry->data;
        MEM_SET_FRAME(pdpt_destentry, pdpt_dest_block);

        // Now map in the existing PDPT
        page_t *pdpt_src = (page_t*)mem_remapPhys(MEM_GET_FRAME(pdpt_srcentry), PAGE_SIZE);

        // Copy PDs
        for (size_t pd = 0; pd < 512; pd++) {
            page_t *pd_srcentry = &pdpt_src[pd];
            if (!(pd_srcentry->bits.present)) continue;

            page_t *pd_destentry = &pdpt_dest[pd];

            // Allocate a new page directory
            uintptr_t pd_dest_block = pmm_allocateBlock();
            page_t *pd_dest  = (page_t*)mem_remapPhys(pd_dest_block, PAGE_SIZE);
            memset((void*)pd_dest, 0, PAGE_SIZE); 
        
            // Do a raw copy but set the frame
            pd_destentry->data = pd_srcentry->data;
            MEM_SET_FRAME(pd_destentry, pd_dest_block);

            // Now map in the existing PD
            page_t *pd_src = (page_t*)mem_remapPhys(MEM_GET_FRAME(pd_srcentry), PAGE_SIZE);

            // Copy PTs
            for (size_t pt = 0; pt < 512; pt++) {
                page_t *pt_srcentry = &pd_src[pt];
                if (!(pt_srcentry->bits.present)) continue;

                page_t *pt_destentry = &pd_dest[pt];

                // Allocate a new page table
                uintptr_t pt_dest_block = pmm_allocateBlock();
                page_t *pt_dest  = (page_t*)mem_remapPhys(pt_dest_block, PAGE_SIZE);
                memset((void*)pt_dest, 0, PAGE_SIZE);

                // Do a raw copy but set the frame
                pt_destentry->data = pt_srcentry->data;
                MEM_SET_FRAME(pt_destentry, pt_dest_block);

                // Now map in the existing PT
                page_t *pt_src = (page_t*)mem_remapPhys(MEM_GET_FRAME(pt_srcentry), PAGE_SIZE);

                // Copy pages
                for (size_t page = 0; page < 512; page++) {
                    page_t *page_src = &pt_src[page];
                    page_t *page_dest = &pt_dest[page];
                    if (!(page_src->bits.present)) continue; // Not present

                    if (page_src->bits.usermode) {
                        uintptr_t address = ((pdpt << (9 * 3 + 12)) | (pd << (9*2 + 12)) | (pt << (9 + 12)) | (page << MEM_PAGE_SHIFT));
                        dprintf(DEBUG, "Usermode page at address %p - stall\n", address);
                        for (;;);
                    } else {
                        // Raw copy
                        page_dest->data = page_src->data;
                    }
                } 
            }
        }
    }
    

    return dest;
}

/**
 * @brief Map a physical address to a virtual address
 * 
 * @param dir The directory to map them in (leave blank for current)
 * @param phys The physical address
 * @param virt The virtual address
 * @param flags Additional flags to use (e.g. MEM_KERNEL)
 */
void mem_mapAddress(page_t *dir, uintptr_t phys, uintptr_t virt, int flags) {
    if (!MEM_IS_CANONICAL(virt)) return;

    page_t *pg = mem_getPage(dir, virt, MEM_CREATE);
    if (pg) {
        mem_allocatePage(pg, MEM_NOALLOC | flags);
        MEM_SET_FRAME(pg, phys);
    }
}


/**
 * @brief Returns the page entry requested
 * @param dir The directory to search. Specify NULL for current directory
 * @param address The virtual address of the page (will be aligned for you if not aligned)
 * @param flags The flags of the page to look for
 * 
 * @warning Specifying MEM_CREATE will only create needed structures, it will NOT allocate the page!
 *          Please use a function such as mem_allocatePage to do that.
 */
page_t *mem_getPage(page_t *dir, uintptr_t address, uintptr_t flags) {
    // Validate that the address is canonical
    if (!MEM_IS_CANONICAL(address)) return NULL;

    // Align the address and get the dircetory
    uintptr_t addr = (address % PAGE_SIZE != 0) ? MEM_ALIGN_PAGE(address) : address;
    page_t *directory = (dir == NULL) ? current_cpu->current_dir : dir;

    // Get the PML4
    page_t *pml4_entry = &(directory[MEM_PML4_INDEX(addr)]);
    if (!pml4_entry->bits.present) {
        if (!flags & MEM_CREATE) goto bad_page;

        // Allocate a new PML4 entry and zero it
        uintptr_t block = pmm_allocateBlock();
        uintptr_t block_remap = mem_remapPhys(block, PMM_BLOCK_SIZE);
        memset((void*)block_remap, 0, PMM_BLOCK_SIZE);

        // Setup the bits in the directory index
        pml4_entry->bits.present = 1;
        pml4_entry->bits.rw = 1;
        pml4_entry->bits.usermode = 1; // !!!: Not upholding security 
        MEM_SET_FRAME(pml4_entry, block);

        mem_unmapPhys(block_remap, PMM_BLOCK_SIZE); // we don't even have to do this
    }

    // Get the PDPT and the entry
    page_t *pdpt = (page_t*)mem_remapPhys(MEM_GET_FRAME(pml4_entry), PMM_BLOCK_SIZE);
    page_t *pdpt_entry = &(pdpt[MEM_PDPT_INDEX(addr)]);
    
    if (!pdpt_entry->bits.present) {
        if (!flags & MEM_CREATE) goto bad_page;

        // Allocate a new PDPT entry and zero it
        uintptr_t block = pmm_allocateBlock();
        uintptr_t block_remap = mem_remapPhys(block, PMM_BLOCK_SIZE);
        memset((void*)block_remap, 0, PMM_BLOCK_SIZE);

        // Setup the bits in the directory index
        pdpt_entry->bits.present = 1;
        pdpt_entry->bits.rw = 1;
        pdpt_entry->bits.usermode = 1; // !!!: Not upholding security 
        MEM_SET_FRAME(pdpt_entry, block);

        mem_unmapPhys(block_remap, PMM_BLOCK_SIZE); // we don't even have to do this
    }

    if (pdpt_entry->bits.size) {
        dprintf(WARN, "Tried to get page from a PDPT that is 1GiB\n");
        return NULL;
    }

    // Get the PD and the entry
    page_t *pd = (page_t*)mem_remapPhys(MEM_GET_FRAME(pdpt_entry), PMM_BLOCK_SIZE);
    page_t *pde = &(pd[MEM_PAGEDIR_INDEX(addr)]);

    if (!pde->bits.present) {
        if (!flags & MEM_CREATE) goto bad_page;

        // Allocate a new PDE and zero it
        uintptr_t block = pmm_allocateBlock();
        uintptr_t block_remap = mem_remapPhys(block, PMM_BLOCK_SIZE);

        memset((void*)block_remap, 0, PMM_BLOCK_SIZE);

        // Setup the bits in the directory index
        pde->bits.present = 1;
        pde->bits.rw = 1;
        pde->bits.usermode = 1; // !!!: Not upholding security 
        MEM_SET_FRAME(pde, block);

        mem_unmapPhys(block_remap, PMM_BLOCK_SIZE); // we don't even have to do this
    }

    if (pde->bits.size) {
        dprintf(WARN, "Tried to get page from a PD that is 2MiB\n");
        return NULL;
    }

    // Get the table
    page_t *table = (page_t*)mem_remapPhys(MEM_GET_FRAME(pde), PMM_BLOCK_SIZE);
    page_t *pte = &(table[MEM_PAGETBL_INDEX(addr)]);

    // Return
    return pte;

bad_page:
    return NULL;
}


/**
 * @brief Allocate a page using the physical memory manager
 * 
 * @param page The page object to use. Can be obtained with mem_getPage
 * @param flags The flags to follow when setting up the page
 * 
 * @note You can also use this function to set bits of a specific page - just specify @c MEM_NOALLOC in @p flags.
 * @warning The function will automatically allocate a PMM block if NOALLOC isn't specified and there isn't a frame already set.
 */
void mem_allocatePage(page_t *page, uintptr_t flags) {
    if (!page) return;

    if (flags & MEM_FREE_PAGE) {
        // Just free the page
        mem_freePage(page);
        return;
    }

    if (!page->bits.address && !(flags & MEM_NOALLOC)) {
        // There isn't a frame configured, and the user wants to allocate one.
        uintptr_t block = pmm_allocateBlock();
        MEM_SET_FRAME(page, block);
    }

    // Configure page bits
    page->bits.present          = (flags & MEM_NOT_PRESENT) ? 0 : 1;
    page->bits.rw               = (flags & MEM_READONLY) ? 0 : 1;
    page->bits.usermode         = (flags & MEM_KERNEL) ? 0 : 1;
    page->bits.writethrough     = (flags & MEM_WRITETHROUGH) ? 1 : 0;
    page->bits.cache_disable    = (flags & MEM_NOT_CACHEABLE) ? 1 : 0;
}

/**
 * @brief Free a page
 * 
 * @param page The page to free
 */
void mem_freePage(page_t *page) {
    if (!page) return;

    // Mark the page as not present
    page->bits.present = 0;
    page->bits.rw = 0;
    page->bits.usermode = 0;
    
    // Free the block
    pmm_freeBlock(MEM_GET_FRAME(page));
    MEM_SET_FRAME(page, 0x0);
}

/**
 * @brief Create an MMIO region
 * @param phys The physical address of the MMIO space
 * @param size Size of the requested space (must be aligned)
 * @returns Address to new mapped MMIO region
 * 
 * @warning MMIO regions cannot be destroyed.
 */
uintptr_t mem_mapMMIO(uintptr_t phys, uintptr_t size) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

/**
 * @brief Allocate a DMA region from the kernel
 * 
 * DMA regions are contiguous blocks that currently cannot be destroyed
 */
uintptr_t mem_allocateDMA(uintptr_t size) {
    // Align size
    if (size % PAGE_SIZE != 0) size = MEM_ALIGN_PAGE(size);

    // Check if we're going to overexpand
    if (mem_dmaRegion + size > MEM_DMA_REGION + MEM_DMA_REGION_SIZE) {
        // We have a problem
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Out of space trying to map DMA region of size 0x%x\n", size);
        __builtin_unreachable();
    }

    spinlock_acquire(&dma_lock);

    // Map into memory
    for (uintptr_t i = mem_dmaRegion; i < mem_dmaRegion + size; i += PAGE_SIZE) {
        page_t *pg = mem_getPage(NULL, i, MEM_CREATE);
        if (pg) mem_allocatePage(pg, MEM_KERNEL | MEM_NOT_CACHEABLE);
    }

    // Update size
    mem_dmaRegion += size;

    spinlock_release(&dma_lock);

    return mem_dmaRegion - size;
}

/**
 * @brief Unallocate a DMA region from the kernel
 * 
 * @param base The address returned by @c mem_allocateDMA
 * @param size The size of the base
 */
void mem_freeDMA(uintptr_t base, uintptr_t size) {
    if (!base || !size) return;

    // Align size
    if (size % PAGE_SIZE != 0) size = MEM_ALIGN_PAGE(size);

    // If this was the last DMA region mapped, we can unmap it
    if (base == mem_dmaRegion - size) {
        // Get the lock
        spinlock_acquire(&dma_lock);

        // Free the pages
        // TODO: Do we not need to actually free pages? Can't we just hack in support in mem_mapDriver?
        mem_dmaRegion -= size;
        for (uintptr_t i = mem_dmaRegion; i < mem_dmaRegion + size; i += PAGE_SIZE) {
            page_t *pg = mem_getPage(NULL, i, MEM_DEFAULT);
            if (pg) mem_freePage(pg);
        }

        // Release the lock
        spinlock_release(&dma_lock);

        return;
    }

    // Else we're out of luck
    dprintf(WARN, "DMA unmapping is not implemented (tried to unmap region %p - %p)\n", base, base+size);
}

/**
 * @brief Map a driver into memory
 * @param size The size of the driver in memory
 * @returns A pointer to the mapped space
 */
uintptr_t mem_mapDriver(size_t size) {
    // Align size
    if (size % PAGE_SIZE != 0) size = MEM_ALIGN_PAGE(size);
    
    if (mem_driverRegion + size > MEM_DRIVER_REGION + MEM_DRIVER_REGION_SIZE) {
        // We have a problem
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Out of space trying to allocate driver of size 0x%x\n", size);
        __builtin_unreachable();
    }

    spinlock_acquire(&driver_lock);

    // Map into memory
    for (uintptr_t i = mem_driverRegion; i < mem_driverRegion + size; i += PAGE_SIZE) {
        page_t *pg = mem_getPage(NULL, i, MEM_CREATE);
        if (pg) mem_allocatePage(pg, MEM_KERNEL);
    }

    // Update size
    mem_driverRegion += size;

    spinlock_release(&driver_lock);

    return mem_driverRegion - size;
}

/**
 * @brief Unmap a driver from memory
 * @param base The base address of the driver in memory
 * @param size The size of the driver in memory
 */
void mem_unmapDriver(uintptr_t base, size_t size) {
    // Align size
    if (size % PAGE_SIZE != 0) size = MEM_ALIGN_PAGE(size);

    // If this was the last driver mapped, we can unmap it
    if (base == mem_driverRegion - size) {
        // Get the lock
        spinlock_acquire(&driver_lock);

        // Free the pages
        // TODO: Do we not need to actually free pages? Can't we just hack in support in mem_mapDriver?
        mem_driverRegion -= size;
        for (uintptr_t i = mem_driverRegion; i < mem_driverRegion + size; i += PAGE_SIZE) {
            page_t *pg = mem_getPage(NULL, i, MEM_DEFAULT);
            if (pg) mem_freePage(pg);
        }

        // Release the lock
        spinlock_release(&driver_lock);

        return;
    }

    // Else we're out of luck
    dprintf(WARN, "Driver unmapping is not implemented (tried to unmap driver %p - %p)\n", base, base+size);
}


/**
 * @brief Remap a PMM address to the identity mapped region
 * @param frame_address The address of the frame to remap
 * @param size The size of the address to remap
 * 
 * @note You need to call @c mem_unmapPhys when you are finished with the address.
 */
uintptr_t mem_remapPhys(uintptr_t frame_address, uintptr_t size) {
    if (size > MEM_PHYSMEM_MAP_SIZE) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "*** Remapping physical address %016llX for %016llX, ran out of space.\n", frame_address, size);
        __builtin_unreachable();
    }

    return frame_address | MEM_PHYSMEM_MAP_REGION;
}

/**
 * @brief Unmap a PMM address in the identity mapped region
 * @param frame_address The address of the frame to unmap, as returned by @c mem_remapPhys
 * @param size The size of the frame to unmap
 */
void mem_unmapPhys(uintptr_t frame_address, uintptr_t size) {
    // No caching system is in place, no unmapping.
}


/**
 * @brief Get the physical address of a virtual address
 * @param dir Can be NULL to use the current directory.
 * @param virtaddr The virtual address
 * 
 * @returns NULL on a PDE not being present or the address
 */
uintptr_t mem_getPhysicalAddress(page_t *dir, uintptr_t virtaddr) {
    if (!MEM_IS_CANONICAL(virtaddr)) return 0x0;

    uintptr_t offset = 0x000; // Offset in case address isn't page aligned
    if (virtaddr & 0xFFF) {
        offset = virtaddr & 0xFFF;
        virtaddr = virtaddr & ~0xFFF;
    } 
    
    page_t *pg = mem_getPage(dir, virtaddr, MEM_DEFAULT);

    if (pg) return MEM_GET_FRAME(pg) + offset;
    return 0x0;
}

/**
 * @brief Initialize the memory management subsystem
 * 
 * This function will identity map the kernel into memory and setup page tables.
 * For x86_64, it also sets up the PMM allocator.
 * 
 * @warning MEM_HEAP_REGION is hardcoded into this - we can probably use mem_mapAddress??
 * @warning This is some pretty messy code, I'm sorry :(
 * 
 * @param mem_size The size of memory (aka highest possible address)
 * @param kernel_addr The first free page after the kernel
 */
void mem_init(uintptr_t mem_size, uintptr_t kernel_addr) {
    // Set the initial page region as the current page for this core.
    current_cpu->current_dir = (page_t*)&mem_kernelPML;

    // 5 level paging? We don't care right now
    mem_use5LevelPaging = cpu_pml5supported();
    if (mem_use5LevelPaging) {
        dprintf(INFO, "5-level paging is supported by this CPU\n");
    } else {
        dprintf(INFO, "5-level paging is not supported by this CPU\n");
    }

    // First, create an identity map. This is important
    // !!!: == THIS IS REALLY BAD (but makes things quick)
    // !!!: We are basically going to use 2MiB pages in the identity map region and not use caching, since it isn't
    // !!!: required. This is bad because most things expect a 4KB page, but 2MiB pages mean that we can fit a lot more.
    // !!!: See https://github.com/klange/toaruos/blob/a54a0cbbee1ac18bceb2371e49876eab9abb3a11/kernel/arch/x86_64/mmu.c#L1048

    // Map high base in PML
    mem_kernelPML[0][511].data = (uintptr_t)&mem_highBasePDPT[0] | 0x07;

    // Identity map from -128GB using 2MiB pages
    for (size_t i = 0; i < MEM_PHYSMEM_MAP_SIZE / PAGE_SIZE_LARGE / 512; i++) {
        mem_highBasePDPT[i].bits.address = ((uintptr_t)&mem_highBasePDs[i]) >> MEM_PAGE_SHIFT;
        mem_highBasePDPT[i].bits.present = 1;
        mem_highBasePDPT[i].bits.rw = 1;

        // Using 512-page blocks
        for (size_t j = 0; j < 512; j++) {
            mem_highBasePDs[i][j].data = ((i << 30) + (j << 21)) | 0x80 | 0x03;
        }
    }

    // Now, map the kernel.
    // Calculate the amount of pages for the kernel to fit in
    uintptr_t kernel_end_aligned = MEM_ALIGN_PAGE(kernel_addr);
    size_t kernel_pages = (kernel_end_aligned >> MEM_PAGE_SHIFT);

    // How many of those pages can fit into PTs?
    // !!!: Weird math
    size_t kernel_pts = (kernel_pages >= 512) ? (kernel_pages / 512) + ((kernel_pages%512) ? 1 : 0) : 1;

    // Sanity check to make sure Hexahedron isn't bloated
    if ((kernel_pts / 512) / 512 > 1) {
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Hexahedron is too big - requires %i PDPTs when 1 is given\n", (kernel_pts / 512) / 512);
        __builtin_unreachable();
    }

    // I'm lazy :)
    if (kernel_pts / 512 > 1) {
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Hexahedron is too big - multiple low base PDs have not been implemented (requires %i PDs)\n", kernel_pts / 512);
        __builtin_unreachable();
    }
    
    // This one I'll probably have to come back and fix
    if (kernel_pts > 3) {
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Hexahedron is too big - >3 low base PTs have not been implemented (requires %i PTs)\n", kernel_pts);
        __builtin_unreachable();
    }

    dprintf(DEBUG, "Kernel will use %i pages (0x%x)\n", kernel_pages, kernel_pages*PAGE_SIZE);

    // Setup hierarchy (note: we don't setup the PML4 map just yet, that would be really bad.)
    mem_lowBasePDPT[0].bits.address = ((uintptr_t)&mem_lowBasePD >> MEM_PAGE_SHIFT);
    mem_lowBasePDPT[0].bits.present = 1;
    mem_lowBasePDPT[0].bits.rw = 1;
    mem_lowBasePDPT[0].bits.usermode = 1;

    // Start mappin' - we have approx. up to 0x600000 to identity map
    for (size_t i = 0; i < kernel_pts; i++) {
        mem_lowBasePD[i].bits.address = ((uintptr_t)&mem_lowBasePT[i * 512] >> MEM_PAGE_SHIFT);
        mem_lowBasePD[i].bits.present = 1;
        mem_lowBasePD[i].bits.rw = 1;

        for (size_t j = 0; j < 512; j++) {
            mem_lowBasePT[(i * 512) + j].bits.address = ((PAGE_SIZE*512) * i + PAGE_SIZE * j) >> MEM_PAGE_SHIFT;
            mem_lowBasePT[(i * 512) + j].bits.present = 1;
            mem_lowBasePT[(i * 512) + j].bits.rw = 1;
        }
    }


    // Now we can map the PML4 and switch out the loader's (stupid 2MiB paging) initial page region with one that's suited for the kernel
    mem_kernelPML[0][0].data = (uintptr_t)&mem_lowBasePDPT[0] | 0x07;

    dprintf(INFO, "Finished identity mapping kernel, mapping heap...\n");

    // Map the heap into the PML
    mem_kernelPML[0][510].bits.address = ((uintptr_t)&mem_heapBasePDPT >> MEM_PAGE_SHIFT);
    mem_kernelPML[0][510].bits.present = 1;
    mem_kernelPML[0][510].bits.rw = 1;
    
    // Calculate the amount of pages required for our PMM
    size_t frame_bytes = PMM_INDEX_BIT((mem_size >> 12) * 8);
    frame_bytes = MEM_ALIGN_PAGE(frame_bytes);
    size_t frame_pages = frame_bytes >> MEM_PAGE_SHIFT;

    if (frame_pages > 512 * 3) {
        // 512 * 3 = size of the heap base provided, so that's not great.
        dprintf(WARN, "Too much memory available - %i pages required for allocation bitmap (max 1536)\n", frame_pages);
        // TODO: We can probably resolve this by just rewriting some parts of mem_mapAddress
    }

    // Setup hierarchy (ugly)
    mem_heapBasePDPT[0].bits.address = ((uintptr_t)&mem_heapBasePD >> MEM_PAGE_SHIFT);
    mem_heapBasePDPT[0].bits.present = 1;
    mem_heapBasePDPT[0].bits.rw = 1;

    mem_heapBasePD[0].bits.address = ((uintptr_t)&mem_heapBasePT[0] >> MEM_PAGE_SHIFT);
    mem_heapBasePD[0].bits.present = 1;
    mem_heapBasePD[0].bits.rw = 1;

    mem_heapBasePD[1].bits.address = ((uintptr_t)&mem_heapBasePT[512] >> MEM_PAGE_SHIFT);
    mem_heapBasePD[1].bits.present = 1;
    mem_heapBasePD[1].bits.rw = 1;

    mem_heapBasePD[2].bits.address = ((uintptr_t)&mem_heapBasePT[1024] >> MEM_PAGE_SHIFT);
    mem_heapBasePD[2].bits.present = 1;
    mem_heapBasePD[2].bits.rw = 1;

    // Map a bunch o' entries
    for (size_t i = 0; i < frame_pages; i++) {
        mem_heapBasePT[i].bits.address = ((kernel_addr + (i << 12)) >> MEM_PAGE_SHIFT);
        mem_heapBasePT[i].bits.present = 1;
        mem_heapBasePT[i].bits.rw = 1;
    }

    // Tada. We've finished setting up our heap, so now we need to use mem_remapPhys to remap our PML.
    current_cpu->current_dir = (page_t*)mem_remapPhys((uintptr_t)current_cpu->current_dir, 0x0); // Size is arbitrary

    // Now that we have a heap mapped, we can allocate frame bytes.
    uintptr_t *frames = (uintptr_t*)MEM_HEAP_REGION;

    // Initialize PMM
    pmm_init(mem_size, frames); 

    // Call back to architecture to mark/unmark memory
    // !!!: Unmarking too much memory. Would kernel_addr work?
    extern void arch_mark_memory(uintptr_t highest_address, uintptr_t mem_size);
    arch_mark_memory(kernel_pts * 512 * PAGE_SIZE, mem_size);

    // Setup kernel heap to point to after frames
    mem_kernelHeap = MEM_HEAP_REGION + frame_bytes;

    // Now that we have finished creating our basic memory system, we can map the kernel code to be R/O.
    // Force map kernel code (text section)
    extern uintptr_t __text_start, __text_end;
    uintptr_t kernel_code_start = (uintptr_t)&__text_start;
    uintptr_t kernel_code_end = (uintptr_t)&__text_end;

    // Align kernel code end (as text section is positioned at the beginning)
    kernel_code_end = kernel_code_end & ~0xFFF; // This should gurantee that we don't overmap into, say, BSS

    for (uintptr_t i = kernel_code_start; i < kernel_code_end; i += PAGE_SIZE) {
        page_t *pg = mem_getPage(NULL, i, MEM_DEFAULT);
        if (pg) pg->bits.rw = 0;
    }

    // Make space for reference counts in kernel heap
    // Reference counts will be initialized when a user PTE is copied.
    size_t refcount_bytes = frame_bytes >> MEM_PAGE_SHIFT;  // One byte per page
    mem_pageReferences = (uint8_t*)mem_sbrk((refcount_bytes & 0xFFF) ? MEM_ALIGN_PAGE(refcount_bytes) : refcount_bytes);
    memset(mem_pageReferences, 0, refcount_bytes);

    dprintf(INFO, "Memory management initialized\n");
}

/**
 * @brief Expand/shrink the kernel heap
 * 
 * @param b The amount of bytes to allocate/free, needs to a multiple of PAGE_SIZE
 * @returns Address of the start of the bytes when allocating, or the previous address when shrinking
 */
uintptr_t mem_sbrk(int b) {// Sanity checks
    if (!mem_kernelHeap) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "Heap not yet ready\n");
        __builtin_unreachable();
    }

    // Passing b as 0 means they just want the current heap address
    if (!b) return mem_kernelHeap;

    // Make sure the passed integer is a multiple of PAGE_SIZE
    if (b % PAGE_SIZE != 0) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "Heap size expansion must be a multiple of 0x%x\n", PAGE_SIZE);
        __builtin_unreachable();
    }


    // Now lock
    spinlock_acquire(&heap_lock);

    // If you need to shrink the heap, you can pass a negative integer
    if (b < 0) {
        for (uintptr_t i = mem_kernelHeap; i >= mem_kernelHeap + b; i -= 0x1000) {
            mem_freePage(mem_getPage(NULL, i, 0));
        }

        uintptr_t oldStart = mem_kernelHeap;
        mem_kernelHeap += b; // Subtracting wouldn't be very good, would it?
        spinlock_release(&heap_lock);
        return oldStart;
    }

    for (uintptr_t i = mem_kernelHeap; i < mem_kernelHeap + b; i += 0x1000) {
        // Check if the page already exists
        page_t *pagechk = mem_getPage(NULL, i, 0);
        if (pagechk && pagechk->bits.present) {
            // hmmm
            dprintf(WARN, "sbrk found odd pages at 0x%x - 0x%x\n", i, i + 0x1000);
            
            // whatever its free memory
            continue;
        }

        page_t *page = mem_getPage(NULL, i, MEM_CREATE);
        mem_allocatePage(page, MEM_KERNEL);
    }

    uintptr_t oldStart = mem_kernelHeap;
    mem_kernelHeap += b;
    spinlock_release(&heap_lock);
    return oldStart;
}