/**
 * @file hexahedron/include/kernel/arch/i386/arch.h
 * @brief Provides basic architecture definitions (internal)
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_I386_ARCH_H
#define KERNEL_ARCH_I386_ARCH_H

/**** INCLUDES ****/
#include <kernel/generic_mboot.h>
#include <kernel/multiboot.h>


/**** FUNCTIONS ****/

/**
 * @brief Say hi! Prints the versioning message and ASCII art to NOHEADER dprintf
 */
void arch_say_hello();


/**
 * @brief Parse a Multiboot 1 header and packs into a @c generic_parameters structure
 * @param bootinfo The Multiboot information
 * @returns A generic parameters structure
 */
generic_parameters_t *arch_parse_multiboot1(multiboot_t *bootinfo);

/**
 * @brief Mark/unmark valid spots in memory
 * @todo Work in tandem with mem.h to allow for a maximum amount of blocks to be used
 */
void arch_mark_memory(generic_parameters_t *parameters, uintptr_t highest_address);


/**
 * @brief Get the generic parameters
 */
generic_parameters_t *arch_get_generic_parameters();

#endif