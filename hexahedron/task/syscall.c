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
    [SYS_OPEN]          = (syscall_func_t)(uintptr_t)sys_open,
    [SYS_READ]          = (syscall_func_t)(uintptr_t)sys_read,
    [SYS_WRITE]         = (syscall_func_t)(uintptr_t)sys_write,
    [SYS_CLOSE]         = (syscall_func_t)(uintptr_t)sys_close,
    [SYS_BRK]           = (syscall_func_t)(uintptr_t)sys_brk
};

/* Unimplemented system call */
#define SYSCALL_UNIMPLEMENTED(syscall) kernel_panic_extended(UNSUPPORTED_FUNCTION_ERROR, "syscall", "*** The system call \"%s\" is unimplemented\n", syscall)

/* Pointer validation */
#define SYSCALL_VALIDATE_PTR(ptr) (mem_validate((void*)ptr, PTR_USER | PTR_STRICT))

/* Log method */
#define LOG(status, ...) dprintf_module(status, "TASK:SYSCALL", __VA_ARGS__)


/**
 * @brief Pointer validation failed
 * @param ptr The pointer that failed to validate
 */
void syscall_pointerValidateFailed(void *ptr) {
    kernel_panic_prepare(KERNEL_BAD_ARGUMENT_ERROR);

    printf("*** Process \"%s\" tried to access an invalid pointer (%p)\n", current_cpu->current_process->name, ptr);
    dprintf(NOHEADER, COLOR_CODE_RED_BOLD "*** Process \"%s\" tried to access an invalid pointer (%p)\n\n" COLOR_CODE_RESET, current_cpu->current_process->name, ptr);

    kernel_panic_finalize();
}


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

/**
 * @brief Close system call
 */
int sys_close(int fd) {
    printf("sys_close fd %d\n", fd);
    return 0;
}

/**
 * @brief Change data segment size
 */
void *sys_brk(void *addr) {
    printf("sys_brk addr %p\n", addr);

    // Validate pointer is in range
    if ((uintptr_t)addr < current_cpu->current_process->heap_base) {
        // brk() syscall should return current program heap location on error
        return (void*)current_cpu->current_process->heap;
    }

    // TODO: Validate resource limit

    // If the user wants to shrink the heap, then do it
    if ((uintptr_t)addr < current_cpu->current_process->heap) {
        size_t free_size = current_cpu->current_process->heap - (uintptr_t)addr;
        mem_free((uintptr_t)addr, free_size, MEM_DEFAULT);
        current_cpu->current_process->heap = (uintptr_t)addr;
        return addr;
    }


    // Else, "handle"
    current_cpu->current_process->heap = (uintptr_t)addr;   // Sure.. you can totally have this memory ;)
                                                            // (page fault handler will map this on a critical failure)


    return addr;
}