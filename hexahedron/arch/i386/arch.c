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

#include <stdint.h>


/**
 * @brief Main architecture function
 * @note Does not return
 */
__attribute__((noreturn)) void arch_main() {
    for (;;);
}