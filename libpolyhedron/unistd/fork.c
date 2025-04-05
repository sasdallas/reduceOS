/**
 * @file libpolyhedron/unistd/fork.c
 * @brief fork syscall
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <unistd.h>

DEFINE_SYSCALL0(fork, SYS_FORK);

pid_t fork() {
    return __syscall_fork();
}