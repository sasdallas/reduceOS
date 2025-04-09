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
    [SYS_BRK]           = (syscall_func_t)(uintptr_t)sys_brk,
    [SYS_FORK]          = (syscall_func_t)(uintptr_t)sys_fork,
    [SYS_LSEEK]         = (syscall_func_t)(uintptr_t)sys_lseek
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
 * @returns Only if resolved.
 */
void syscall_pointerValidateFailed(void *ptr) {

    // Check to see if this pointer is within process heap boundary
    if ((uintptr_t)ptr >= current_cpu->current_process->heap_base && (uintptr_t)ptr < current_cpu->current_process->heap) {
        // Yep, it's valid. Map a page
        mem_allocatePage(mem_getPage(NULL, (uintptr_t)ptr, MEM_CREATE), MEM_DEFAULT);
        return;
    }

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
    // LOG(INFO, "Received system call %d\n", syscall->syscall_number);

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
    LOG(DEBUG, "sys_exit %d\n", status);
    process_exit(NULL, status);
}

/**
 * @brief Open system call
 */
int sys_open(const char *pathname, int flags, mode_t mode) {
    // Validate pointer
    if (SYSCALL_VALIDATE_PTR(pathname) == 0) {
        syscall_pointerValidateFailed((void*)pathname);
    }

    // Try and get it open
    fs_node_t *node = kopen(pathname, flags);

    // Did we find the node and they DIDN'T want us to create it?
    if (node && (flags & O_CREAT) && (flags & O_EXCL)) {
        fs_close(node);
        return -EEXIST;
    }

    // Did we find it and did they want to create it?
    if (!node && (flags & O_CREAT)) {
        // Yes... uh, what?
        // I guess this means that kopen ignored the flag, so assume EROFS?
        LOG(WARN, "Failed to create \"%s\" - assuming read-only file system\n", pathname);
        return -EROFS;
    }

    // Did they want a directory?
    if (node && !(node->flags & VFS_DIRECTORY) && (flags & O_DIRECTORY)) {
        fs_close(node);
        return -ENOTDIR;
    }

    // Did we find it and they want it?
    if (!node) {
        return -ENOENT;
    }

    // Create the file descriptor and return
    fd_t *fd = fd_add(current_cpu->current_process, node);
    
    // Are they trying to append? If so modify length to be equal to node length
    if (flags & O_APPEND) {
        fd->offset = node->length;
    }

    LOG(DEBUG, "sys_open %s flags %d mode %d\n", pathname, flags, mode);
    return fd->fd_number;
}

/**
 * @brief Read system call
 */
ssize_t sys_read(int fd, void *buffer, size_t count) {
    if (SYSCALL_VALIDATE_PTR(buffer) == 0) {
        syscall_pointerValidateFailed((void*)buffer);
    }

    if (!FD_VALIDATE(current_cpu->current_process, fd)) {
        return -EBADF;
    }

    fd_t *proc_fd = FD(current_cpu->current_process, fd);
    ssize_t i = fs_read(proc_fd->node, proc_fd->offset, count, (uint8_t*)buffer);
    proc_fd->offset += i;

    // LOG(DEBUG, "sys_read fd %d buffer %p count %d\n", fd, buffer, count);
    return i;
}

/**
 * @brief Write system calll
 */
ssize_t sys_write(int fd, const void *buffer, size_t count) {
    if (SYSCALL_VALIDATE_PTR(buffer) == 0) {
        syscall_pointerValidateFailed((void*)buffer);
    }

    // stdout?
    if (fd == STDOUT_FILE_DESCRIPTOR) {
        char *buf = (char*)buffer;
        buf[count] = 0;
        printf("%s", buffer);
        return count;
    }

    if (!FD_VALIDATE(current_cpu->current_process, fd)) {
        return -EBADF;
    }

    fd_t *proc_fd = FD(current_cpu->current_process, fd);
    ssize_t i = fs_write(proc_fd->node, proc_fd->offset, count, (uint8_t*)buffer);
    proc_fd->offset += i;

    LOG(DEBUG, "sys_write fd %d buffer %p count %d\n", fd, buffer, count);
    return i;
}

/**
 * @brief Close system call
 */
int sys_close(int fd) {
    LOG(DEBUG, "sys_close fd %d\n", fd);
    return 0;
}

/**
 * @brief Change data segment size
 */
void *sys_brk(void *addr) {
    LOG(DEBUG, "sys_brk addr %p\n", addr);

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

/**
 * @brief Fork system call
 */
pid_t sys_fork() {
    return process_fork();
}

/**
 * @brief lseek system call
 */
off_t sys_lseek(int fd, off_t offset, int whence) {
    LOG(DEBUG, "sys_lseek %d %d %d\n", fd, offset, whence);

    if (!FD_VALIDATE(current_cpu->current_process, fd)) {
        return -EBADF;
    }

    // Handle whence
    if (whence == SEEK_SET) {
        FD(current_cpu->current_process, fd)->offset = offset;
    } else if (whence == SEEK_CUR) {
        FD(current_cpu->current_process, fd)->offset += offset;
    } else if (whence == SEEK_END) {
        // TODO: This is the problem area (offset > node length)
        FD(current_cpu->current_process, fd)->offset = FD(current_cpu->current_process, fd)->node->length + offset;
    } else {
        return -EINVAL;
    }


    // TODO: What if offset > file size? We don't have proper safeguards...
    return FD(current_cpu->current_process, fd)->offset;
}