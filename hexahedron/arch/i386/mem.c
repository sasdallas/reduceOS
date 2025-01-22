/**
 * @file hexahedron/arch/i386/mem.c
 * @brief i386 memory subsystem
 * 
 * @todo A locking subsystem NEEDS to be implemented
 * @todo Reference bitmap for pages and cloning functions, but usermode is far away.
 * @todo Map pool can use a trick from x86_64 and use 4MiB pages
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

// Standard includes
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

// Memory includes
#include <kernel/mem/mem.h>
#include <kernel/arch/i386/mem.h>
#include <kernel/mem/pmm.h>

// General kernel includes
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/processor_data.h>
#include <kernel/arch/arch.h>
#include <kernel/misc/pool.h>
#include <kernel/misc/spinlock.h>


// Current & kernel directories
static page_t       *mem_kernelDirectory;       // Kernel page directory
                                                // TODO: We can stack allocate and align this.

// Reference counts
uint8_t     *mem_pageReferences     = NULL;     // Holds a bitmap of references

// Heap/identity map stuff
uintptr_t    mem_kernelHeap;                    // Location of the kernel heap in memory
uintptr_t    mem_identityMapCacheSize;          // Size of our actual identity map (it is basically a cache)
pool_t       *mem_mapPool = NULL;               // Identity map pool

// Regions
uintptr_t       mem_mmioRegion      = MEM_MMIO_REGION;     // Memory-mapped I/O region
uintptr_t       mem_driverRegion    = MEM_DRIVER_REGION;   // Driver region
uintptr_t       mem_dmaRegion       = MEM_DMA_REGION;      // DMA region

// Spinlocks (stack-allocated - no spinlock for ID map is required as pool system handles that)
static spinlock_t heap_lock = { 0 };
static spinlock_t mmio_lock = { 0 };
static spinlock_t driver_lock = { 0 };
static spinlock_t dma_lock = { 0 };


/**
 * @brief Get the current position of the kernel heap
 * @returns The current position of the kernel heap
 */
uintptr_t mem_getKernelHeap() {
    return mem_kernelHeap;
}

/**
 * @brief Invalidate a page in the TLB
 * @param addr The address of the page 
 * @warning This function is only to be used when removing P-V mappings. Just free the page if it's identity.
 */
static inline void mem_invalidatePage(uintptr_t addr) {
    asm volatile ("invlpg (%0)" :: "r"(addr) : "memory");
    // TODO: When SMP is done, a TLB shootdown will happen here (slow)
    // TODO: SMP is done, implement TLB shootdown
} 

/**
 * @brief Load new value into the PDBR
 * @param pagedir The address of the page directory to load
 */
static inline void mem_load_pdbr(uintptr_t addr) {
    asm volatile ("mov %0, %%cr3" :: "r"(addr));
}

/**
 * @brief Enable/disable paging
 * @param status Enable/disable
 */
void mem_setPaging(bool status) {
    if (status) {
        // Turn on paging
        uint32_t cr0, cr4;

        asm volatile ("mov %%cr4, %0" : "=r"(cr4));
        cr4 = cr4 & 0xFFFFFFEF; // Clear PSE bit
        asm volatile ("mov %0, %%cr4" :: "r"(cr4));

        asm volatile ("mov %%cr0, %0" : "=r"(cr0));
        cr0 = cr0 | 0x80010001; // Enable paging
        asm volatile ("mov %0, %%cr0" :: "r"(cr0));
    } else {
        uint32_t cr0;

        asm volatile ("mov %%cr0, %0" : "=r"(cr0));
        cr0 = cr0 & ~CR0_PG_BIT; // Disable paging
        asm volatile ("mov %0, %%cr4" :: "r"(cr0));
    }
}

/**
 * @brief Get the current page directory
 */
page_t *mem_getCurrentDirectory() {
    return current_cpu->current_dir;
}

/**
 * @brief Get the kernel page directory
 */
page_t *mem_getKernelDirectory() {
    return mem_kernelDirectory;
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
int mem_switchDirectory(page_t *pagedir) {
    if (!pagedir) return -EINVAL;

    // Load into current directory
    current_cpu->current_dir = pagedir;

    // Load PDBR
    mem_load_pdbr((uintptr_t)pagedir & ~MEM_PHYSMEM_CACHE_REGION);

    return 0;
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
 * @brief Maybe copy a usermode page
 * @param src_pt The source page table
 * @param dest_pt The destination page table
 * 
 * @ref https://github.com/klange/toaruos/blob/master/kernel/arch/x86_64/mmu.c
 * 
 * @returns The block address of the table
 */
static void mem_copyUserPage(page_t *src, page_t *dest) {

    // Check if the source page is writable
    if (src->bits.rw) {
        // It is, initialize reference counts for the page's frame
        if (mem_pageReferences[src->bits.address]) {
            // There's already references??
            kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem_copyonwrite", "*** Source page already has references\n");
            __builtin_unreachable();
        }

        // 2 references
        mem_pageReferences[src->bits.address] = 2;

        // Now what we do is mark the source AND destination page as R/O and set them both to have a CoW pending
        // Any writes will trigger a page fault, which our handler will detect and auto handle CoW
        src->bits.rw = 0;
        src->bits.cow = 1;

        // Raw copy to destination
        dest->data = src->data;

        // All done!
        // TODO: Invalidate the page with TLB shootdown
        dprintf(WARN, "IMPLEMENT TLB SHOOTDOWN\n");
        return;
    }

    // It's not writable. Can we add a new reference?
    if (mem_incrementPageReference(src) == 0) {
        // There are too many reference counts. We need to create a copy of the page.
        // We can do this by mapping the old page into memory, creating a new page, copying contents, and then done
        uintptr_t src_frame = mem_remapPhys(MEM_GET_FRAME(src), PAGE_SIZE);
        uintptr_t dest_frame_block = pmm_allocateBlock();
        uintptr_t dest_frame = mem_remapPhys(dest_frame_block, PAGE_SIZE);
        memcpy((void*)dest_frame, (void*)src_frame, PAGE_SIZE);

        // Setup bits
        dest->data = src->data;
        MEM_SET_FRAME(dest, dest_frame_block);
        dest->bits.cow = 0; // Not CoW

        mem_unmapPhys(dest_frame, PAGE_SIZE);
        mem_unmapPhys(src_frame, PAGE_SIZE);
        return;
    }

    // Yes, we can. Raw copy and return
    dest->data = src->data;
}


/**
 * @brief Clone a page directory.
 * 
 * This is a full PROPER page directory clone.
 * This function does it properly and clones the page directory, its tables, and their respective entries fully.
 * It also has the option to do CoW on usermode pages
 * 
 * @param dir The source page directory. Keep as NULL to clone the current page directory.
 * @returns The page directory on success
 */
page_t *mem_clone(page_t *dir) {
    if (!dir) dir = mem_getCurrentDirectory();

    // Get our return directory
    page_t *dest = mem_createVAS();

    // Now start copying PDEs
    for (int pde = 0; pde < 1024; pde++) {
        page_t *src_pde = &dir[pde];
        if (!(src_pde->bits.present)) continue; // PDE isn't present

        // Construct a new table and add it to our output
        uintptr_t dest_pt_block = pmm_allocateBlock();
        page_t *dest_pt = (page_t*)mem_remapPhys(dest_pt_block, PMM_BLOCK_SIZE);
        memset((void*)dest_pt, 0, PMM_BLOCK_SIZE);

        // Get the PDE in our new VAS and set it up to point to new_pt
        page_t *dest_pde = &dest[pde];
        
        // Setup the bits
        dest_pde->data = src_pde->data; // Do a raw copy first
        MEM_SET_FRAME(dest_pde, dest_pt_block);

        // Now get the source PT
        page_t *src_pt = (page_t*)mem_remapPhys((uintptr_t)MEM_GET_FRAME(src_pde), PMM_BLOCK_SIZE);

        for (int pte = 0; pte < 1024; pte++) {
            page_t *src_pte = &src_pt[pte];
            if (!(src_pte->bits.present)) continue; // Not present

            page_t *dest_pte = &dest_pt[pte];

            // Is it a usermode page? We need to do CoW in that case
            if (src_pte->bits.usermode) {
                mem_copyUserPage(src_pte, dest_pte);
            } else {
                // Just do raw copy
                dest_pte->data = src_pte->data;
            }
        }

        // Cleanup and unmap
        mem_unmapPhys((uintptr_t)src_pt, PMM_BLOCK_SIZE);
        mem_unmapPhys((uintptr_t)dest_pt, PMM_BLOCK_SIZE);
    }
    

    return dest;
}

/**
 * @brief Remap a PMM address to the identity mapped region
 * @param frame_address The address of the frame to remap
 * @param size The size of the address to remap
 */
uintptr_t mem_remapPhys(uintptr_t frame_address, uintptr_t size) {    
    if (frame_address + size < mem_identityMapCacheSize) {
        return frame_address | MEM_PHYSMEM_CACHE_REGION;
    }

    if (mem_mapPool == NULL) {
        // Initialize the map pool! We'll allocate a pool to the address.
        // !!!: There is a potential for a disaster if mem_getPage tries to remap phys. and the pool hasn't been initialized.  
        // !!!: Luckily this system is abstracted enough that we can fix this, hopefully.
        // !!!: However if the allocator hasn't been initialized, we are so screwed.
        mem_mapPool = pool_create("map pool", PAGE_SIZE, MEM_PHYSMEM_MAP_SIZE, MEM_PHYSMEM_MAP_REGION, POOL_DEFAULT);
        dprintf(INFO, "Physical memory identity map pool created (0x%x - 0x%x)\n", MEM_PHYSMEM_MAP_REGION, MEM_PHYSMEM_MAP_REGION + MEM_PHYSMEM_MAP_SIZE);
    }

    uintptr_t offset = 0; // This is the offset which will be added to the allocated chunks (in order to result in a frame address equal to the one passed in)

    if (size % PAGE_SIZE != 0) {
        size = MEM_ALIGN_PAGE(size);
        // kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "*** Bad size to remapPhys: 0x%x\n", size);
    }

    if (frame_address % PAGE_SIZE != 0) {
        // We know that frame_address + size will overflow into more pages (e.g. frame address 0x7FE1900 + 0x1000 actually requires 2 pages for 0x7FE1900 - 0x7FE2000)
        // !!!: this is wasteful and stupid, just like this whole system.
        offset = frame_address & 0xFFF;
        frame_address = frame_address & ~0xFFF;
        size += PAGE_SIZE;

        // kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "*** Bad frame to remapPhys: 0x%x\n", frame_address);
    }
    
    // Now try to get a pool address
    uintptr_t start_addr = pool_allocateChunks(mem_mapPool, size / PAGE_SIZE);
    if (start_addr == (uintptr_t)NULL) {
        // We've run out of space in the identity map. That's not great!
        // There should be a system in place to prevent this - it should just trigger a generic OOM error but this is also here to prevent overruns.
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Too much physical memory is in use. Reached the maximum size of the identity mapped region (call 0x%x size 0x%x).\n", frame_address, size);
        __builtin_unreachable();
    }

    for (uintptr_t i = frame_address; i < frame_address + size; i += PAGE_SIZE) {
        // Do this manually
        mem_mapAddress(NULL, i, start_addr + (i - frame_address), MEM_KERNEL);
    }

    return start_addr + offset;
}

/**
 * @brief Unmap a PMM address in the identity mapped region
 * @param frame_address The address of the frame to unmap, as returned by @c mem_remapPhys
 * @param size The size of the frame to unmap
 */
void mem_unmapPhys(uintptr_t frame_address, uintptr_t size) {
    if (frame_address < MEM_PHYSMEM_CACHE_REGION) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "mem", "*** 0x%x < 0x%x\n", frame_address, MEM_PHYSMEM_CACHE_REGION);
    }

    if ((frame_address - MEM_PHYSMEM_CACHE_REGION) + size < mem_identityMapCacheSize) {
        return; // No work to be done. It's in the cache.
    }

    if (size % PAGE_SIZE != 0) {
        size = MEM_ALIGN_PAGE(size);
    }

    if (frame_address % PAGE_SIZE != 0) {
        frame_address = frame_address & ~0xFFF;
        size += PAGE_SIZE;
    }

    // See if we can just free those chunks. Because mem_remapPhys doesn't actually use pmm_allocateBlock,
    // we have no need to mess with pages
    pool_freeChunks(mem_mapPool, frame_address, size / PAGE_SIZE);
}

/**
 * @brief Get the physical address of a virtual address
 * @param dir Can be NULL to use the current directory.
 * @param virtaddr The virtual address
 * 
 * @returns NULL on a PDE not being present or the address
 */
uintptr_t mem_getPhysicalAddress(page_t *dir, uintptr_t virtaddr) {
    page_t *directory = (dir == NULL) ? current_cpu->current_dir : dir;
    
    // Create an offset
    uintptr_t offset = (virtaddr & 0xFFF);

    // Get the directory entry and its corresponding table
    uintptr_t addr = (virtaddr % PAGE_SIZE == 0) ? virtaddr : (virtaddr & ~0xFFF);
    page_t *pde = &(directory[MEM_PAGEDIR_INDEX(addr)]);
    if (pde->bits.present == 0) {
        // The PDE wasn't present
        return (uintptr_t)NULL;
    }

    // Remember to remap any frames to that identity map area.
    page_t *table = (page_t*)(mem_remapPhys(MEM_GET_FRAME(pde), PMM_BLOCK_SIZE)); 
    page_t *pte = &(table[MEM_PAGETBL_INDEX(addr)]);
    
    mem_unmapPhys((uintptr_t)table, PMM_BLOCK_SIZE);

    return MEM_GET_FRAME(pte) + offset;
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
    page_t *directory = (dir == NULL) ? current_cpu->current_dir : dir;

    // Get the page
    page_t *page = mem_getPage(directory, virt, MEM_CREATE);

    // "Allocate" it but don't set a frame, instead set it to phys
    mem_allocatePage(page, MEM_NOALLOC | flags);
    MEM_SET_FRAME(page, phys);
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
    uintptr_t addr = (address % PAGE_SIZE != 0) ? MEM_ALIGN_PAGE(address) : address;

    page_t *directory = (dir) ? dir : current_cpu->current_dir;

    // Page addresses are divided into 3 parts:
    // - The index of the PDE (bits 22-31)
    // - The index of the PTE (bits 12-21)
    // - The page offset (bits 0-11)

    // Check if the PDE is present
    page_t *pde = &(directory[MEM_PAGEDIR_INDEX(addr)]);
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
        
        mem_unmapPhys(block_remap, PMM_BLOCK_SIZE);
    }

    // Calculate the table index (complex bc MEM_GET_FRAME is for pointers, and I'm lazy)
    page_t *table = (page_t*)mem_remapPhys((uintptr_t)(directory[MEM_PAGEDIR_INDEX(addr)].bits.address << MEM_PAGE_SHIFT), PMM_BLOCK_SIZE);

    page_t *ret = &(table[MEM_PAGETBL_INDEX(addr)]);
    mem_unmapPhys((uintptr_t)table, PMM_BLOCK_SIZE);

    // Return the page
    return ret;

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
    if (size % PAGE_SIZE != 0) kernel_panic(KERNEL_BAD_ARGUMENT_ERROR, "mem");

    if (mem_mmioRegion + size > MEM_MMIO_REGION + MEM_MMIO_REGION_SIZE) {
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Out of space for MMIO allocation\n");
        __builtin_unreachable();
    }

    spinlock_acquire(&mmio_lock);

    uintptr_t address = mem_mmioRegion;
    for (uintptr_t i = 0; i < size; i += PAGE_SIZE) {
        page_t *page = mem_getPage(NULL, address + i, MEM_CREATE);
        if (page) {
            MEM_SET_FRAME(page, (phys + i));
            mem_allocatePage(page, MEM_KERNEL | MEM_WRITETHROUGH | MEM_NOT_CACHEABLE | MEM_NOALLOC);
        }
    }
    
    mem_mmioRegion += size;

    spinlock_release(&mmio_lock);
    return address;
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
    if (mem_driverRegion + size > MEM_DRIVER_REGION + MEM_DRIVER_REGION_SIZE) {
        // We have a problem
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Out of space trying to allocate driver of size 0x%x\n", size);
        __builtin_unreachable();
    }

    spinlock_acquire(&driver_lock);

    // Align size
    if (size % PAGE_SIZE != 0) size = MEM_ALIGN_PAGE(size);

    // Map into memory
    for (uintptr_t i = mem_driverRegion; i < mem_driverRegion + size; i += PAGE_SIZE) {
        page_t *pg = mem_getPage(NULL, i, MEM_CREATE);
        mem_allocatePage(pg, MEM_KERNEL);
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
            page_t *pg = mem_getPage(NULL, i, MEM_CREATE);
            mem_freePage(pg);
        }

        // Release the lock
        spinlock_release(&driver_lock);

        return;
    }

    // Else we're out of luck
    dprintf(WARN, "Driver unmapping is not implemented (tried to unmap driver %p - %p)\n", base, base+size);
}

/**
 * @brief Initialize the memory management subsystem
 * 
 * This function will setup the memory map and prepare tables.
 * It expects you to provide the highest kernel adress that is valid.
 * 
 * @param high_address The ending address of the kernel including all data structures copied.
 */
void mem_init(uintptr_t high_address) {
    if (!high_address) kernel_panic(KERNEL_BAD_ARGUMENT_ERROR, "mem");
    mem_kernelHeap = MEM_ALIGN_PAGE(high_address);

    // Get ourselves a page directory
    // !!!: Is this okay? Do we need to again put things in data structures?
    page_t *page_directory = (page_t*)pmm_allocateBlock();
    memset(page_directory, 0, PMM_BLOCK_SIZE);

    
    // We only have access to 4GB of VAS because of 32-bit protected mode
    // If and when PAE support is implemented into the kernel, we'd get a little more but some machines
    // have much more than that anyways. We need to access PMM memory or else we'll fault everything out of existence.
    // Hexahedron uses a memory map that has mapped PMM memory accessible through a range,
    // which is limited to something like 786 MB (see arch/i386/mem.h)

    // !!!: This sucks. The cache implementation is very finnicky and addresses aren't properly mapped. This entire system needs a full overhaul.
    // !!!: Calculating frame_bytes by multiplying max blocks by PMM_BLOCK_SIZE doesn't give the highest address in memory.

    size_t frame_bytes = pmm_getMaximumBlocks() * PMM_BLOCK_SIZE;

    if (frame_bytes > MEM_PHYSMEM_CACHE_SIZE) {
        dprintf(WARN, "Too much memory in PMM bitmap. Maximum allowed memory size is %i KB and found %i KB - limiting size\n", MEM_PHYSMEM_CACHE_SIZE / 1024, frame_bytes / 1024);
        frame_bytes = MEM_PHYSMEM_CACHE_SIZE;
    }


    size_t frame_pages = (frame_bytes >> MEM_PAGE_SHIFT);

    // Update size
    mem_identityMapCacheSize = frame_bytes; 

    // Identity map!
    uintptr_t frame = 0x0;
    uintptr_t table_frame = 0x0;
    int loop_cycles = (frame_pages + 1024) / 1024;
    int pages_mapped = 0;

    for (int i = 0; i < loop_cycles; i++) {
        page_t *page_table = (page_t*)pmm_allocateBlock();
        memset(page_table, 0, PMM_BLOCK_SIZE);

        for (int pg = 0; pg < 1024; pg++) {
            page_t page = {.data = 0};
            page.bits.present = 1;
            page.bits.rw = 1;
            page.bits.address = (frame >> MEM_PAGE_SHIFT);
            
            page_table[MEM_PAGETBL_INDEX(frame + MEM_PHYSMEM_CACHE_REGION)] = page;

            pages_mapped++;
            if (pages_mapped == (int)(frame_pages)) break;
            frame += 0x1000;
        }
        
        // Create a PDE
        page_t *pde = &(page_directory[MEM_PAGEDIR_INDEX(table_frame + MEM_PHYSMEM_CACHE_REGION)]);
        pde->bits.present = 1;
        pde->bits.rw = 1;
        MEM_SET_FRAME(pde, page_table);

        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)frame_pages) break;
    }

    uintptr_t heap_start_aligned = ((uintptr_t)mem_kernelHeap + 0xFFF) & ~0xFFF;
    uintptr_t kernel_pages = (uintptr_t)heap_start_aligned >> MEM_PAGE_SHIFT;

    // Calculate cycles and reset values
    loop_cycles = (kernel_pages + 1024) / 1024;
    pages_mapped = 0;

    frame = 0x0;
    table_frame = 0x0;

    for (int i = 0; i < loop_cycles; i++) {
        page_t *page_table = (page_t*)pmm_allocateBlock();
        memset(page_table, 0, PMM_BLOCK_SIZE);

        for (int pg = 0; pg < 1024; pg++) {
            page_t page = {.data = 0};
            page.bits.present = 1;
            page.bits.rw = 1;
            page.bits.address = (frame >> MEM_PAGE_SHIFT);
            
            page_table[MEM_PAGETBL_INDEX(frame)] = page;

            pages_mapped++;
            if (pages_mapped == (int)(kernel_pages)) break;

            frame += 0x1000;
        }
        
        // Create a PDE
        page_t *pde = &(page_directory[MEM_PAGEDIR_INDEX(table_frame)]);
        pde->bits.present = 1;
        pde->bits.rw = 1;
        MEM_SET_FRAME(pde, page_table);

        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)kernel_pages) break;
    }

    // All done mapping for now. The memory map should look something like this:
    // 0x00000000 - 0x00400000 is kernel code (-RW)
    // 0xB0000000 - 0xBFFFFFFF is PMM mapped memory (URW)
    // !! PMM mapped memory is exposed. Very bad.

    dprintf(INFO, "Finished creating memory map.\n");
    dprintf(DEBUG, "\tKernel code is from 0x0 - 0x%x\n", high_address);
    dprintf(DEBUG, "\tKernel heap will begin at 0x%x\n", mem_kernelHeap);

    mem_kernelDirectory = page_directory;
    mem_switchDirectory(mem_kernelDirectory);
    mem_setPaging(true);

    // Make space for reference counts in kernel heap
    // Reference counts will be initialized when a user PTE is copied.'
    // NOTE: This has to be done here because mem_sbrk calls mem_getPage which in turn can wrap into mem_remapPhys' map pool system
    size_t refcount_bytes = frame_bytes >> MEM_PAGE_SHIFT;  // One byte per page
    mem_pageReferences = (uint8_t*)mem_sbrk((refcount_bytes & 0xFFF) ? MEM_ALIGN_PAGE(refcount_bytes) : refcount_bytes);
    memset(mem_pageReferences, 0, refcount_bytes);

    dprintf(INFO, "Memory system online and enabled.\n");
}


/**
 * @brief Expand/shrink the kernel heap
 * 
 * @param b The amount of bytes to allocate/free, needs to a multiple of PAGE_SIZE
 * @returns Address of the start of the bytes when allocating, or the previous address when shrinking
 */
uintptr_t mem_sbrk(int b) {
    // Sanity checks
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