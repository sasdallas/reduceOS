/**
 * @file hexahedron/include/kernel/arch/x86_64/interrupt.h
 * @brief x86_64 interrupt handler
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_X86_64_INTERRUPT_H
#define KERNEL_ARCH_X86_64_INTERRUPT_H

/**** INCLUDES ****/
#include <stdint.h>

/**** TYPES ****/

// Interrupt/exception handlers
typedef int (*interrupt_handler_t)(uint32_t, uint32_t, void*, void*);
typedef int (*exception_handler_t)(uint32_t, void*, void*);




#endif