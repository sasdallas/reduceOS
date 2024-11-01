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

// General
#include <kernel/config.h>
#include <kernel/multiboot.h>
#include <kernel/debug.h>

// Architecture-specific
#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/arch.h>

// Generic drivers
#include <kernel/drivers/serial.h>



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
    
    dprintf(NOHEADER, "\tCompiled by %s on %s %s\n", __kernel_compiler, __kernel_build_date, __kernel_build_time);
}

/**
 * @brief Main architecture function
 * @returns Does not return
 */
__attribute__((noreturn)) void arch_main(multiboot_t *bootinfo, uint32_t multiboot_magic, void *esp) {
    // Initialize the HAL. This sets up interrupts & more.
    hal_init();
    
    for (;;);
}