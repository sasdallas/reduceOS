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


/**** FUNCTIONS ****/

/**
 * @brief Say hi! Prints the versioning message and ASCII art to NOHEADER dprintf
 */
void arch_say_hello();


#endif