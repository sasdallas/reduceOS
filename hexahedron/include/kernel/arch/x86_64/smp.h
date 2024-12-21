/**
 * @file hexahedron/include/kernel/arch/x86_64/smp.h
 * @brief Symmetric multiprocessing header file
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_X86_64_SMP_H
#define KERNEL_ARCH_X86_64_SMP_H

/**** INCLUDES ****/
#include <stdint.h>


/**** DEFINITIONS ****/

// IMPORTANT: This value controls the maximum amount of CPUs Hexahedron supports
#define MAX_CPUS            32

// IMPORTANT: This value controls the maximum amount of supported interrupt overrides
#define MAX_INT_OVERRIDES   24

#endif