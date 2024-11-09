/**
 * @file hexahedron/arch/i386/mem.c
 * @brief i386 memory subsystem
 * 
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
#include <kernel/arch/arch.h>


static page_t       *mem_currentDirectory;      // Current page directory. Contains a list of page table entries (PTEs).
static page_t       *mem_kernelDirectory;       // Kernel page directory
                                                // ! We're using this correctly, right?

static uintptr_t    mem_kernelHeap;             // Location of the kernel heap in memory


/**
 * @brief Die in the cold winter
 * @param bytes How many bytes were trying to be allocated
 * @param seq The sequence of failure
 */
void mem_outofmemory(int bytes, char *seq) {
    // Prepare to fault
    kernel_panic_prepare();

    // Print out debug messages
    dprintf(NOHEADER, "*** The memory manager failed to allocate enough memory.\n");
    dprintf(NOHEADER, "*** Failed to allocate %i bytes (sequence: %s)\n", bytes, seq);

    // Finish
    kernel_panic_finalize();
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
 * @brief Remap a PMM address to the identity mapped region
 * @param frame_address The address of the frame to remap
 */
uintptr_t mem_remapPhys(uintptr_t frame_address) {    
    if (frame_address > MEM_IDENTITY_MAP_SIZE) {
        // We've run out of space in the identity map. That's not great!
        // There should be a system in place to prevent this - it should just trigger a generic OOM error but this is also here to prevent overruns.
        kernel_panic_extended(MEMORY_MANAGEMENT_ERROR, "mem", "*** Too much physical memory is in use. Reached the maximum size of the identity mapped region.");
        __builtin_unreachable();
    }

    return frame_address | MEM_IDENTITY_MAP_REGION;
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
    mem_currentDirectory = pagedir;

    // Load PDBR
    mem_load_pdbr((uintptr_t)pagedir & ~MEM_IDENTITY_MAP_REGION);

    return 0;
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
page_t *mem_getPage(page_t *dir, uintptr_t addr, uintptr_t flags) {
    page_t *directory = (dir) ? dir : mem_currentDirectory;

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
        memset((void*)mem_remapPhys(block), 0, PAGE_SIZE);

        dprintf(DEBUG, "Creating a PDE at 0x%x\n", block);

        // Setup the bits in the directory index
        pde->bits.present = 1;
        pde->bits.rw = 1;
        pde->bits.usermode = 1; // FIXME: Not upholding security 
        pde->bits.address = (block >> MEM_PAGE_SHIFT);
    }

    // Calculate the table index
    page_t *table = (page_t*)((uintptr_t)(directory[MEM_PAGEDIR_INDEX(addr)].bits.address << MEM_PAGE_SHIFT));
    
    // Return the page
    return &(table[MEM_PAGETBL_INDEX(addr)]);

bad_page:
    return NULL;
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
    mem_kernelHeap = high_address;

    // Get ourselves a page directory
    // !!!: Is this okay? Do we need to again put things in data structures?
    page_t *page_directory = (page_t*)pmm_allocateBlock();
    memset(page_directory, 0, PMM_BLOCK_SIZE);

    
    // We only have access to 4GB of VAS because of 32-bit protected mode
    // If and when PAE support is implemented into the kernel, we'd get a little more but some machines
    // have much more than that anyways. We need to access PMM memory or else we'll fault everything out of existence.
    // Hexahedron uses a memory map that has mapped PMM memory accessible through a range,
    // which is limited to something like 786 MB (see arch/i386/mem.h)

    size_t frame_bytes = pmm_getMaximumBlocks() * PMM_BLOCK_SIZE;
    size_t frame_pages = (frame_bytes >> MEM_PAGE_SHIFT);

    if (frame_bytes > MEM_IDENTITY_MAP_SIZE) {
        dprintf(WARN, "Too much memory in PMM bitmap. Maximum allowed memory size is %i KB and found %i KB - limiting size\n", MEM_IDENTITY_MAP_SIZE / 1024, frame_bytes / 1024);
    }

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
            page.bits.usermode = 1; // uhhhhh
            page.bits.address = (frame >> MEM_PAGE_SHIFT);
            
            page_table[MEM_PAGETBL_INDEX(frame + MEM_IDENTITY_MAP_REGION)] = page;

            pages_mapped++;
            if (pages_mapped == (int)(frame_pages)) break;
            frame += 0x1000;
        }
        
        // Create a PDE
        page_t *pde = &(page_directory[MEM_PAGEDIR_INDEX(table_frame + MEM_IDENTITY_MAP_REGION)]);
        pde->bits.present = 1;
        pde->bits.rw = 1;
        pde->bits.usermode = 1; // uhhhhhhhhhh
        pde->bits.address = ((uintptr_t)page_table >> MEM_PAGE_SHIFT);
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
            if (pages_mapped == (int)(frame_pages)) break;

            frame += 0x1000;
        }
        
        // Create a PDE
        page_t *pde = &(page_directory[MEM_PAGEDIR_INDEX(table_frame)]);
        pde->bits.present = 1;
        pde->bits.rw = 1;
        pde->bits.address = ((uintptr_t)page_table >> MEM_PAGE_SHIFT);
        table_frame += 0x1000 * 1024;

        if (pages_mapped == (int)frame_pages) break;
    }

    // All done mapping for now. The memory map should look something like this:
    // 0x00000000 - 0x00400000 is kernel code (-RW)
    // 0xB0000000 - 0xBFFFFFFF is PMM mapped memory (URW)
    // !! PMM mapped memory is exposed. Very bad.

    dprintf(INFO, "Finished creating memory map.\n");
    mem_kernelDirectory = page_directory;
    mem_switchDirectory(mem_kernelDirectory);
    mem_setPaging(true);
}