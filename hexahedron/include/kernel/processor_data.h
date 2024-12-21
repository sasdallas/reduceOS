/**
 * @file hexahedron/include/kernel/processor_data.h
 * @brief Implements a generic and architecture-specific CPU core data system
 * 
 * The architectures that implement SMP need to update this structure with their own 
 * data fields. Generic data fields (like the current process, cpu_id, etc.) are required as well. 
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_PROCESSOR_DATA_H
#define KERNEL_PROCESSOR_DATA_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/arch/arch.h>
#include <kernel/mem/mem.h>

/**** TYPES ****/

typedef struct _processor {
    int cpu_id;
    page_t *current_dir;

#if defined(__ARCH_X86_64__) || defined(__ARCH_I386__)
    int lapic_id;

    /* CPU basic information */
    char cpu_model[48];
    const char *cpu_manufacturer;
    int cpu_model_number;
    int cpu_family;
#endif
} processor_t;


/* External variables defined by architecture */
extern processor_t processor_data[];
extern int processor_count;

/**
 * @brief Architecture-specific method of determining current core
 * 
 * i386: We use a macro that retrieves the current data from processor_data
 * x86_64: We use the GSbase to get it
 */

#if defined(__ARCH_I386__)
#define current_cpu ((processor_t*)&(processor_data[arch_current_cpu()]))
#elif defined(__ARCH_X86_64__)
processor_t __seg_gs * const current_cpu = 0;
#else
#error "Please define a method of getting processor data"
#endif


#endif