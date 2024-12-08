/**
 * @file hexahedron/include/kernel/arch/arch.h
 * @brief Provides general architecture-specific definitions.
 * 
 * Certain architectures need to expose certain functions. All of that is contained in this file.
 * Each function should have three things:
 * - A clear description of what it takes
 * - A clear description of what it does
 * - A clear description of what it returns
 * 
 * @note Includes HAL definitions as well!
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_H
#define KERNEL_ARCH_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/generic_mboot.h>

/**** FUNCTIONS ****/

/**
 * @brief Prepare the architecture to enter a fatal state. This means cleaning up registers,
 * moving things around, whatever you need to do
 */
extern void arch_panic_prepare();


/**
 * @brief Finish handling the panic, clean everything up and halt.
 * @note Does not return
 */
extern void arch_panic_finalize();

/**
 * @brief Get the generic parameters
 */
extern generic_parameters_t *arch_get_generic_parameters();

/**
 * @brief Returns the current CPU active in the system
 */
int arch_current_cpu();

#endif