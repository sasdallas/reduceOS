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
#include <kernel/task/process.h>
#include <kernel/fs/vfs.h>
#include <kernel/debug.h>
#include <kernel/panic.h>

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

/* System call table */
static syscall_func_t syscall_table[] = {
    [SYS_EXIT]          = (syscall_func_t)(uintptr_t)sys_exit,
    // TODO: SYS_GETEUID
    [SYS_OPEN]          = (syscall_func_t)(uintptr_t)sys_open,
    [SYS_READ]          = (syscall_func_t)(uintptr_t)sys_read,
    [SYS_WRITE]         = (syscall_func_t)(uintptr_t)sys_write
};

/* Unimplemented system call */
#define SYSCALL_UNIMPLEMENTED(syscall) kernel_panic_extended(UNSUPPORTED_FUNCTION_ERROR, "syscall", "*** The system call \"%s\" is unimplemented\n", syscall)

/* Log method */
#define LOG(status, ...) dprintf_module(status, "TASK:SYSCALL", __VA_ARGS__)


/**
 * @brief Handle a system call
 * @param syscall The system call to handle
 * @returns Nothing, but updates @c syscall->return_value
 */
void syscall_handle(syscall_t *syscall) {
    LOG(INFO, "Received system call %d\n", syscall->syscall_number);

    // Is the system call within bounds?
    if (syscall->syscall_number < 0 || syscall->syscall_number >= (int)(sizeof(syscall_table) / sizeof(*syscall_table))) {
        LOG(ERR, "Invalid system call %d received\n");
        syscall->return_value = -EINVAL;
        return;
    }

    // Call!
    syscall->return_value = (syscall_table[syscall->syscall_number])(
                                syscall->parameters[0], syscall->parameters[1], syscall->parameters[2],
                                syscall->parameters[3], syscall->parameters[4]);

    return;
}

/**
 * @brief Exit system call
 */
void sys_exit(int status) {
    printf("sys_exit %d\n", status);
    process_exit(NULL, status);
}

/**
 * @brief Open system call
 */
int sys_open(const char *pathname, int flags, mode_t mode) {
    printf("sys_open %s flags %d mode %d\n", pathname, flags, mode);
    return -EACCES;
}

/**
 * @brief Read system calll
 */
ssize_t sys_read(int fd, void *buffer, size_t count) {
    printf("sys_read fd %d buffer %p count %d\n", fd, buffer, count);
    return 0;
}

/**
 * @brief Write system calll
 */
ssize_t sys_write(int fd, const void *buffer, size_t count) {
    if (fd == 1) {
        // stdout
        printf("%s", buffer);
        return count;
    }

    printf("sys_write fd %d buffer %p count %d\n", fd, buffer, count);
    return 0;
}
