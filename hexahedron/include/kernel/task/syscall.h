/**
 * @file hexahedron/include/kernel/task/syscall.h
 * @brief System call handler
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

/**** INCLUDES ****/
#include <stdint.h>

/**** DEFINITIONS ****/

#define SYSCALL_MAX_PARAMETERS      6

/**** TYPES ****/

/**
 * @brief System call structure
 */
typedef struct syscall {
    int syscall_number;
    uint32_t parameters[SYSCALL_MAX_PARAMETERS];
} syscall_t;

/**** FUNCTIONS ****/

/**
 * @brief Handle a system call
 * @param syscall The system call to handle
 * @returns Value
 */
int syscall_handle(syscall_t *syscall);



#endif