/**
 * @file hexahedron/arch/x86_64/mem.c
 * @brief Memory management functions for x86_64
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

/* Stub variables */
uintptr_t mem_mapPool = 0xAAAAAAAAAAAAAAAA;
uintptr_t mem_identityMapCacheSize = 0xAAAAAAAAAAAAAAAA;
uintptr_t mem_kernelHeap = 0xAAAAAAAAAAAAAAAA;

/* Whether to use 5-level paging */
static int mem_use5LevelPaging = 0;

/* Base page layout - loader uses this */
page_t mem_kernelPML[3][512] __attribute__((aligned(PAGE_SIZE))) = {0};


/* Low base PDPT/PD/PT (identity mapping space for kernel/other stuff) */
page_t mem_lowBasePDPT[512] __attribute__((aligned(PAGE_SIZE))) = {0}; 
page_t mem_lowBasePD[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_lowBasePT[512*3] __attribute__((aligned(PAGE_SIZE))) = {0};

/* Heap PDPT/PD/PT */
page_t mem_heapBasePDPT[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_heapBasePD[512] __attribute__((aligned(PAGE_SIZE))) = {0};
page_t mem_heapBasePT[512*3] __attribute__((aligned(PAGE_SIZE))) = {0};





/**
 * @brief Map a physical address to a virtual address
 * 
 * @param dir The directory to map them in (leave blank for current)
 * @param phys The physical address
 * @param virt The virtual address
 */
void mem_mapAddress(page_t *dir, uintptr_t phys, uintptr_t virt) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

/**
 * @brief Expand/shrink the kernel heap
 * 
 * @param b The amount of bytes to allocate/free, needs to a multiple of PAGE_SIZE
 * @returns Address of the start of the bytes when allocating, or the previous address when shrinking
 */
uintptr_t mem_sbrk(int b) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
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
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
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
 * @brief Initialize the memory management subsystem
 * 
 * This function will identity map the kernel into memory and setup page tables.
 * For x86_64, it also sets up the PMM allocator.
 * 
 * @param mem_size The size of memory (aka highest possible address)
 * @param kernel_addr The first free page after the kernel
 */
void mem_init(uintptr_t mem_size, uintptr_t kernel_addr) {
    // Set the initial page region as the current page for this core.
    current_cpu->current_dir = (page_t*)&mem_kernelPML;

    mem_use5LevelPaging = cpu_pml5supported();
    if (mem_use5LevelPaging) {
        dprintf(INFO, "5-level paging is supported by this CPU\n");
    } else {
        dprintf(INFO, "5-level paging is not supported by this CPU\n");
    }

    // Calculate the amount of pages for the kernel to fit in
    extern uintptr_t __kernel_end;
    uintptr_t kernel_end_aligned = MEM_ALIGN_PAGE((uintptr_t)&__kernel_end);
    size_t kernel_pages = (kernel_end_aligned >> MEM_PAGE_SHIFT);

    dprintf(DEBUG, "Kernel requires %i pages\n", kernel_pages);

    // How many of those pages can fit into PTs?
    size_t kernel_pts = (kernel_pages / 512) ? (kernel_pages / 512) : 1;

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


    // Now we can map the PML4
    mem_kernelPML[0][0].data = (uintptr_t)&mem_lowBasePDPT[0] | 0x07;

    dprintf(INFO, "Finished identity mapping kernel, mapping heap\n");

    // Map the heap into the PML
    mem_kernelPML[0][510].bits.address = ((uintptr_t)&mem_heapBasePDPT >> MEM_PAGE_SHIFT);
    mem_kernelPML[0][510].bits.present = 1;
    mem_kernelPML[0][510].bits.rw = 1;
    
    // Calculate the amount of pages required for our PMM
    size_t frame_bytes = mem_size / PMM_BLOCK_SIZE;
    frame_bytes = MEM_ALIGN_PAGE(frame_bytes);
    size_t frame_pages = frame_bytes >> MEM_PAGE_SHIFT;

    

    if (frame_pages > 512 * 3) {
        // 512 * 3 = size of the heap base, so that's not great.
        dprintf(WARN, "Too much memory available - %i pages required for allocation bitmap (max 1536)\n", frame_pages);
    }

    // Setup hierarchy
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

    


}