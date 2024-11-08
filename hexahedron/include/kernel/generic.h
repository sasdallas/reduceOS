/**
 * @file hexahedron/include/kernel/generic.h
 * @brief Provides definitions for some generic structures used around the kernel.
 * 
 * Generic structures are structures that have information that non-architecture specific parts of
 * Hexahedron require. They are freestanding, and are required to provide if needed.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_GENERIC_H
#define KERNEL_GENERIC_H

/**** INCLUDES ****/
#include <stdint.h>

/**** TYPES ****/

// Generic module descriptor
typedef struct _generic_module_descriptor {
    uint32_t mod_start;     // Starting address of the module
    uint32_t mod_end;       // Ending address of the module
    char *cmdline;          // Command-line options passed to the module
} generic_module_descriptor_t;

// Generic parameters passed to the kernel. This is useful when loading.
typedef struct _generic_parameters {
    /* Kernel load options */
    char *kernel_cmdline;
    char *bootloader_name; // OPTIONAL: Leave as NULL to not provide

    /* Modules */
    generic_module_descriptor_t *modules; // TODO: Replace with list
    int nmodules;
} generic_parameters_t;


#endif