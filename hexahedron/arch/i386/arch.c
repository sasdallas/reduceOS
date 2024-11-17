/**
 * @file hexahedron/arch/i386/arch.c
 * @brief Architecture startup header for I386
 * 
 * This file handles the beginning initialization of everything specific to this architecture.
 * For I386, it sets up things like interrupts, TSSes, SMP cores, etc.
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

// Polyhedron
#include <stdint.h>
#include <string.h>

// General
#include <kernel/config.h>
#include <kernel/multiboot.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/pmm.h>
#include <kernel/generic_mboot.h>
#include <kernel/misc/spinlock.h>

// Architecture-specific
#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/arch.h>

// Generic drivers
#include <kernel/drivers/serial.h>

// Parameters
generic_parameters_t *parameters;

/**
 * @brief Say hi! Prints the versioning message and ASCII art to NOHEADER dprintf
 */
void arch_say_hello() {
    // Print out a hello message
    dprintf(NOHEADER, "%s\n", __kernel_ascii_art_formatted);
    dprintf(NOHEADER, "Hexahedron %d.%d.%d-%s-%s (codename \"%s\")\n", 
                    __kernel_version_major, 
                    __kernel_version_minor, 
                    __kernel_version_lower, 
                    __kernel_architecture,
                    __kernel_build_configuration,
                    __kernel_version_codename);
    
    dprintf(NOHEADER, "\tCompiled by %s on %s %s\n\n", __kernel_compiler, __kernel_build_date, __kernel_build_time);
}

/**
 * @brief Prepare the architecture to enter a fatal state. This means cleaning up registers,
 * moving things around, whatever you need to do
 */
void arch_panic_prepare() {

}

/**
 * @brief Finish handling the panic, clean everything up and halt.
 * @note Does not return
 */
void arch_panic_finalize() {
    // Disable interrupts & halt
    asm volatile ("cli\nhlt");
    for (;;);
}


/**
 * @brief Get the generic parameters
 */
generic_parameters_t *arch_get_generic_parameters() {
    return parameters;
}


extern uintptr_t __bss_end;
static uintptr_t highest_kernel_address = ((uintptr_t)&__bss_end);  // This is ONLY used until memory management is initialized.
                                                                    // mm will take over this

/**
 * @brief Zeroes and allocates bytes for a structure at the end of the kernel
 * @param bytes Amount of bytes to allocate
 * @returns The address to which the structure can be placed at 
 */
uintptr_t arch_allocate_structure(size_t bytes) {
    uintptr_t ptr = highest_kernel_address;
    highest_kernel_address += bytes;

    memset((void*)ptr, 0x00, bytes);
    return ptr;
}

/**
 * @brief Copy & relocate a structure to the end of the kernel.
 * @param structure_ptr A pointer to the structure
 * @param size The size of the structure
 * @returns The address to which it was relocated.
 */
uintptr_t arch_relocate_structure(uintptr_t structure_ptr, size_t size) {
    uintptr_t location = arch_allocate_structure(size);
    memcpy((void*)location, (void*)structure_ptr, size);
    return location;
}


/**
 * @brief Main architecture function
 * @returns Does not return
 */
__attribute__((noreturn)) void arch_main(multiboot_t *bootinfo, uint32_t multiboot_magic, void *esp) {
    // Initialize the HAL in stage #1. This sets up interrupts & more.
    hal_init(HAL_STAGE_1);

    // Align the kernel address
    highest_kernel_address += PAGE_SIZE;
    highest_kernel_address &= ~0xFFF;

    
    
    // Let's get some multiboot information.
    parameters = NULL;

    if (multiboot_magic == MULTIBOOT_MAGIC) {
        dprintf(INFO, "Found a Multiboot1 structure\n");
        parameters = arch_parse_multiboot1(bootinfo);
    } else if (multiboot_magic == MULTIBOOT2_MAGIC) {
        dprintf(INFO, "Found a Multiboot2 structure\n");
        kernel_panic(KERNEL_DEBUG_TRAP, "arch");
    } else {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "arch", "*** Unknown multiboot structure when checking kernel.\n");
    }

    dprintf(INFO, "Loaded by '%s' with command line '%s'\n", parameters->bootloader_name, parameters->kernel_cmdline);
    dprintf(INFO, "Available physical memory to machine: %i KB\n", parameters->mem_size);

    // Start the PMM system
    uintptr_t *pmm_frames = (uintptr_t*)arch_allocate_structure((parameters->mem_size * 1024) / PMM_BLOCK_SIZE);
    pmm_init(parameters->mem_size * 1024, pmm_frames);

    // Mark memory as valid/invalid. 
    arch_mark_memory(parameters, highest_kernel_address);

    extern uint32_t __text_start;
    uintptr_t kernel_end = (uintptr_t)&__bss_end;
    uintptr_t kernel_start = (uintptr_t)&__text_start;
    dprintf(INFO, "Kernel is using %i KB in memory - extra datastructures are using %i KB\n", (kernel_end - kernel_start) / 1024, (highest_kernel_address - kernel_end) / 1024);



    // The memory subsystem is SEPARATE from the HAL.
    // This is intentional because it requires that we place certain objects such as multiboot information,
    // ACPI tables, PMM bitmaps, etc. all below the required factor.    
    mem_init(highest_kernel_address);
    
    allocator_info_t *info = alloc_getInfo();
    dprintf(INFO, "Allocator information: %s version %i.%i (valloc %s, profiling %s)\n", info->name, info->version_major, info->version_minor,
                                            info->support_valloc ? "supported" : "not supported",
                                            info->support_profile ? "supported" : "not supported");


    // We're clear to perform the second part of HAL startup
    hal_init(HAL_STAGE_2);

    for (;;);
}