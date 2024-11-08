/**
 * @file hexahedron/include/kernel/panic.h
 * @brief Kernel panic header
 * 
 * Defines all stopcodes & panic functions
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H


/**** INCLUDES ****/
#include <stdint.h>

/**** STOP CODES ****/

#define     KERNEL_STOP_CODES               3

#define     KERNEL_DEBUG_TRAP               0x00000000 // Debugging trap
#define     MEMORY_MANAGEMENT_ERROR         0x00000001 // Memory management failure
#define     KERNEL_BAD_ARGUMENT_ERROR       0x00000002 // Critical function was passed a bad argument


/**** MESSAGES ****/

extern char *kernel_panic_messages[KERNEL_STOP_CODES];
extern char *kernel_bugcode_strings[KERNEL_STOP_CODES];

extern int __kernel_stop_codes;

/**** FUNCTIONS ****/

/**
 * @brief Immediately panic and stop the kernel.
 * @param bugcode Takes in a kernel bugcode, e.g. KERNEL_DEBUG_TRAP
 * @param module The module the fault occurred in, e.g. "vfs"
 */
void kernel_panic(uint32_t bugcode, char *module);

/**
 * @brief Immediately panic and stop the kernel.
 * 
 * @param bugcode Takes in a kernel bugcode, e.g. KERNEL_DEBUG_TRAP
 * @param module The module the fault occurred in, e.g. "vfs"
 * @param format String to print out (this is va_args, you may use formatting)
 */
void kernel_panic_extended(uint32_t bugcode, char *module, char *format, ...);



#endif