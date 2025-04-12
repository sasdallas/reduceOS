/**
 * @file libpolyhedron/include/unistd.h
 * @brief unistd
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <sys/cheader.h>
_Begin_C_Header

#ifndef _UNISTD_H
#define _UNISTD_H

/**** INCLUDES ****/
#include <sys/syscall.h>
#include <sys/syscall_defs.h>
#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

/**** FUNCTIONS ****/

void exit(int status);
int open(const char *pathname, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
int brk(void *addr);
void *sbrk(intptr_t increment);
pid_t fork();
off_t lseek(int fd, off_t offset, int whence);

int usleep(useconds_t usec);
int execve(const char *pathname, const char *argv[], char *envp[]);

/* STUBS */
int mkdir(const char *pathname, mode_t mode);
int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int system(const char *command);

#endif

_End_C_Header