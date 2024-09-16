/**
 * @file ioctl.c
 * @brief Handles I/O control requests.
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <errno.h>

#ifdef errno
#undef errno
extern int errno;
#endif


int ioctl(int fd, int request, void *argp) {
    __sets_errno(SYSCALL3(int, SYS_IOCTL, fd, request, argp));
}