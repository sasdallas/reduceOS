/**
 * @file syscalls.c
 * @brief reduceOS system calls for Newlib.
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 *
 * @copyright 
 * The Newlib C library and other components such as libgloss are not governed by the reduceOS BSD license.
 * The licenses for the software contained in this library can be found in "COPYING.NEWLIB"
 * For more information, please contact the maintainers of the package.
 *
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include "sys/ioctl.h"
#include "sys/syscall.h"

#include <errno.h>
#undef errno
extern int errno;

void _exit(int status_code) {
    SYSCALL1(int, SYS_EXIT, status_code);
    __builtin_unreachable();
}

int close(int file) { __sets_errno(SYSCALL1(int, SYS_CLOSE, file)); }

int execve(char* name, char** argv, char** env) { __sets_errno(SYSCALL3(int, SYS_EXECVE, name, argv, env)); }

int fork() { __sets_errno(SYSCALL0(int, SYS_FORK)); }

int fstat(int file, struct stat* st) { __sets_errno(SYSCALL2(int, SYS_FSTAT, file, st)); }

int getpid() { return SYSCALL0(int, SYS_GETPID); }

int isatty(int file) {
    // No system calls
    int dtype = ioctl(file, 0x4F00, NULL); // 0x4F00 = IOCTLDTYPE
    if (dtype == IOCTL_DTYPE_TTY) return 1;
    errno = EINVAL;
    return 0;
}

int link(char* old, char* new) {
    __sets_errno(SYSCALL2(int, SYS_LINK, old, new));
}

int lseek(int file, int ptr, int dir) { __sets_errno(SYSCALL3(int, SYS_LSEEK, file, ptr, dir)); }

int open(const char* name, int flags, ...) {
    // TODO: va_args?
    __sets_errno(SYSCALL2(int, SYS_OPEN, name, flags));
}

int read(int file, char* ptr, int len) {
    __sets_errno(SYSCALL3(int, SYS_READ, file, ptr, len));
}

int write(int file, char* ptr, int len) {
    __sets_errno(SYSCALL3(int, SYS_WRITE, file, ptr, len));
}

caddr_t sbrk(int incr) {
    __sets_errno(SYSCALL1(caddr_t, SYS_SBRK, incr));
}

int stat(const char* __restrict __path, struct stat* __restrict st) { return SYSCALL2(int, SYS_STAT, __path, st); }

clock_t times(struct tms* buf) { __sets_errno(SYSCALL1(unsigned long, SYS_TIMES, buf)); }

int unlink(char* name) {
    __sets_errno(SYSCALL1(int, SYS_UNLINK, name));
}

int wait(int* status) { __sets_errno(SYSCALL1(int, SYS_WAIT, status)); }

int mkdir(const char* pathname, mode_t mode) { __sets_errno(SYSCALL2(int, SYS_MKDIR, (char*)pathname, mode)); }

int waitpid(pid_t pid, int* status, int options) { __sets_errno(SYSCALL3(int, SYS_WAITPID, pid, status, options)); }
