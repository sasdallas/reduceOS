/**
 * @file hexahedron/arch/x86_64/util.c
 * @brief Utility functions provided to generic parts of kernel
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/x86_64/arch.h>
#include <kernel/arch/x86_64/hal.h>
#include <kernel/arch/arch.h>
#include <kernel/processor_data.h>

/* External parameters */
extern generic_parameters_t *parameters;

/**
 * @brief Returns the current CPU active in the system
 */
int arch_current_cpu() {
    return current_cpu->cpu_id;
}

/**
 * @brief Get the generic parameters
 */
generic_parameters_t *arch_get_generic_parameters() {
    return parameters;
}

/**
 * @brief Pause execution on the current CPU for one cycle
 */
void arch_pause() {
    asm volatile ("pause");
}

/**
 * @brief Prepare to switch to a new thread
 * @param thread The thread to prepare to switch to
 */
void arch_prepare_switch(struct thread *thread) {
    // TODO: Implement, need to load TSS stack
}

