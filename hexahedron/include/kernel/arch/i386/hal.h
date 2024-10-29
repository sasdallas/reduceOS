/**
 * @file hexahedron/include/kernel/arch/i386/hal.h
 * @brief Architecture-specific HAL functions
 * 
 * HAL functions that need to be called from other parts of the architecture (e.g. hardware-specific drivers)
 * are implemented here.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_I386_HAL_H
#define KERNEL_ARCH_I386_HAL_H

/**** INCLUDES ****/
#include <stdint.h>

/**** FUNCTIONS ****/
 

/**
 * @brief Initialize the hardware abstraction layer
 * 
 * Initializes serial output, memory systems, and much more for I386.
 */
void hal_init();

#endif