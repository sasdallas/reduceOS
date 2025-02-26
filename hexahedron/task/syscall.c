/**
 * @file hexahedron/task/syscall.c
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

#include <kernel/task/syscall.h>
#include <kernel/debug.h>

/* Log method */
#define LOG(status, ...) dprintf_module(status, "TASK:SYSCALL", __VA_ARGS__)


/**
 * @brief Handle a system call
 * @param syscall The system call to handle
 * @returns Value
 */
int syscall_handle(syscall_t *syscall) {
    LOG(INFO, "Received system call %d\n", syscall->syscall_number);
    return 0;
}