/**
 * @file libpolyhedron/unistd/usleep.c
 * @brief usleep
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <sys/syscall.h>
#include <unistd.h>

DEFINE_SYSCALL1(usleep, SYS_USLEEP, useconds_t);

int usleep(useconds_t usec) {
    __sets_errno(__syscall_usleep(usec));
}