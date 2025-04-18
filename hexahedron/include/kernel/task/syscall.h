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
#include <sys/types.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>

/**** DEFINITIONS ****/

#define SYSCALL_MAX_PARAMETERS      6       // !!!: AT THE CURRENT TIME, ONLY 5 ARE SUPPORTED

/**** TYPES ****/

/**
 * @brief System call structure
 */
typedef struct syscall {
    int syscall_number;
    long parameters[SYSCALL_MAX_PARAMETERS];
    long return_value;
} syscall_t;

/**
 * @brief System call function
 * 
 * @warning We're treading in unknown waters here - overloading functions
 */
typedef long (*syscall_func_t)(long, long, long, long, long);

/**
 * @brief waitpid context
 */
typedef struct waitpid_context {
    struct process *process;    // Process we are waiting for
    int options;                // Options
    int *wstatus;               // Wait status
    
} waitpid_context_t;

/**** FUNCTIONS ****/

/**
 * @brief Handle a system call
 * @param syscall The system call to handle
 * @returns Nothing, but updates @c syscall->return_value
 */
void syscall_handle(syscall_t *syscall);

/* System calls */
void sys_exit(int status);
int sys_open(const char *pathname, int flags, mode_t mode);
ssize_t sys_read(int fd, void *buffer, size_t count);
ssize_t sys_write(int fd, const void *buffer, size_t count);
int sys_close(int fd);
long sys_stat(const char *pathname, struct stat *statbuf);
long sys_fstat(int fd, struct stat *statbuf);
long sys_lstat(const char *pathname, struct stat *statbuf);
long sys_ioctl(int fd, unsigned long request, void *argp);
void *sys_brk(void *addr);
pid_t sys_fork();
off_t sys_lseek(int fd, off_t offset, int whence);
long sys_gettimeofday(struct timeval *tv, void *tz);
long sys_settimeofday(struct timeval *tv, void *tz);
long sys_usleep(useconds_t usec);
long sys_execve(const char *pathname, const char *argv[], const char *envp[]);
long sys_wait(pid_t pid, int *wstatus, int options);

#endif