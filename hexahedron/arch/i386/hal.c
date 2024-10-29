/**
 * @file hexahedron/arch/i386/hal.c
 * @brief Hardware abstraction layer for I386
 * 
 * This implements a hardware abstraction system for the architecture.
 * No specific calls should be made from generic. This includes interrupt handlers, port I/O, etc.
 * 
 * Generic HAL calls are implemented in kernel/hal.h
 * Architecture-specific HAL calls are implemented in kernel/arch/<architecture>/hal.h
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/hal.h>
#include <kernel/hal.h>


/**
 * @brief Initialize the hardware abstraction layer
 * 
 * Initializes serial output, memory systems, and much more for I386.
 */
void hal_init() {
    
}