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
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/gfx/gfx.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* System call table */
static syscall_func_t syscall_table[] = {
    [SYS_EXIT]          = (syscall_func_t)(uintptr_t)sys_exit,
    [SYS_OPEN]          = (syscall_func_t)(uintptr_t)sys_open,
    [SYS_READ]          = (syscall_func_t)(uintptr_t)sys_read,
    [SYS_WRITE]         = (syscall_func_t)(uintptr_t)sys_write,
    [SYS_CLOSE]         = (syscall_func_t)(uintptr_t)sys_close,
    [SYS_STAT]          = (syscall_func_t)(uintptr_t)sys_stat,
    [SYS_FSTAT]         = (syscall_func_t)(uintptr_t)sys_fstat,
    [SYS_LSTAT]         = (syscall_func_t)(uintptr_t)sys_lstat,
    [SYS_BRK]           = (syscall_func_t)(uintptr_t)sys_brk,
    [SYS_FORK]          = (syscall_func_t)(uintptr_t)sys_fork,
    [SYS_LSEEK]         = (syscall_func_t)(uintptr_t)sys_lseek,
    [SYS_GETTIMEOFDAY]  = (syscall_func_t)(uintptr_t)sys_gettimeofday,
    [SYS_SETTIMEOFDAY]  = (syscall_func_t)(uintptr_t)sys_settimeofday,
    [SYS_USLEEP]        = (syscall_func_t)(uintptr_t)sys_usleep,
    [SYS_EXECVE]        = (syscall_func_t)(uintptr_t)sys_execve,
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
        video_updateScreen();
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
    if (!FD_VALIDATE(current_cpu->current_process, fd)) {
        return -EBADF;
    }

    LOG(DEBUG, "sys_close fd %d\n", fd);
    fd_remove(current_cpu->current_process, fd);
    return 0;
}

/**
 * @brief Common stat for stat/fstat/lstat
 * @param f The file to check
 * @param statbuf The stat buffer to use
 */
static void sys_stat_common(fs_node_t *f, struct stat *statbuf) {
    // Convert VFS flags to st_dev
    if (f->flags == VFS_DIRECTORY)      statbuf->st_dev |= S_IFDIR; // Directory
    if (f->flags == VFS_BLOCKDEVICE)    statbuf->st_dev |= S_IFBLK; // Block device
    if (f->flags == VFS_CHARDEVICE)     statbuf->st_dev |= S_IFCHR; // Character device
    if (f->flags == VFS_FILE)           statbuf->st_dev |= S_IFREG; // Regular file
    if (f->flags == VFS_SYMLINK)        statbuf->st_dev |= S_IFLNK; // Symlink
    if (f->flags == VFS_PIPE)           statbuf->st_dev |= S_IFIFO; // FIFO or not, it's a pipe
    if (f->flags == VFS_SOCKET)         statbuf->st_dev |= S_IFSOCK; // Socket
    if (f->flags == VFS_MOUNTPOINT)     statbuf->st_dev |= S_IFDIR; // ???

    // st_mode is just st_dev with extra steps
    statbuf->st_mode = statbuf->st_dev;

    // Setup other fields
    statbuf->st_ino = f->inode; // Inode number
    statbuf->st_mode |= f->mask; // File mode - TODO: Make sure that file mode is properly set with vaild mask bits
    statbuf->st_nlink = 0; // TODO
    statbuf->st_uid = f->uid;
    statbuf->st_gid = f->gid;
    statbuf->st_rdev = 0; // TODO
    statbuf->st_size = f->length;
    statbuf->st_blksize = STAT_DEFAULT_BLOCK_SIZE; // TODO: This would prove useful for file I/O
    statbuf->st_blocks = 0; // TODO
    statbuf->st_atime = f->atime;
    statbuf->st_mtime = f->mtime;
    statbuf->st_ctime = f->ctime;
}

/**
 * @brief Stat system call
 */
long sys_stat(const char *pathname, struct stat *statbuf) {
    if (!SYSCALL_VALIDATE_PTR(pathname)) syscall_pointerValidateFailed((void*)pathname); // TODO: EFAULT
    if (!SYSCALL_VALIDATE_PTR(statbuf)) syscall_pointerValidateFailed((void*)statbuf);

    // Try to open the file
    fs_node_t *f = kopen(pathname, O_RDONLY); // TODO: return ELOOP, O_NOFOLLOW is supposed to work but need to refine this
    if (!f) return -ENOENT;

    // Common stat
    sys_stat_common(f, statbuf);

    // Close the file
    fs_close(f);

    // Done
    return 0;
}

/**
 * @brief fstat system call
 */
long sys_fstat(int fd, struct stat *statbuf) {
    if (!FD_VALIDATE(current_cpu->current_process, fd)) return -EBADF;
    if (!SYSCALL_VALIDATE_PTR(statbuf)) syscall_pointerValidateFailed((void*)statbuf);

    // Try to do stat
    sys_stat_common(FD(current_cpu->current_process, fd)->node, statbuf);
    return 0;
}

/**
 * @brief lstat system call
 */
long sys_lstat(const char *pathname, struct stat *statbuf) {
    if (!SYSCALL_VALIDATE_PTR(pathname)) syscall_pointerValidateFailed((void*)pathname); // TODO: EFAULT
    if (!SYSCALL_VALIDATE_PTR(statbuf)) syscall_pointerValidateFailed((void*)statbuf);

    // Try to open the file
    fs_node_t *f = kopen(pathname, O_NOFOLLOW | O_PATH);    // Get actual link file
                                                            // TODO: Handle open errors?
    
    if (!f) return -ENOENT;

    // Common stat
    sys_stat_common(f, statbuf);

    // Close the file
    fs_close(f);

    // Done
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

/**
 * @brief Get time of day system call
 * @todo Use struct timezone
 */
long sys_gettimeofday(struct timeval *tv, void *tz) {
    if (!SYSCALL_VALIDATE_PTR(tv)) {
        // TODO: Resolve by returning -EFAULT
        syscall_pointerValidateFailed((void*)tv);
    }

    if (tz && !SYSCALL_VALIDATE_PTR(tz)) {
        // TODO: Resolve by returning -EFAULT
        syscall_pointerValidateFailed(tz);
    }

    return gettimeofday(tv, tz);
}

/**
 * @brief Set time of day system call
 * @todo Use struct timezone
 */
long sys_settimeofday(struct timeval *tv, void *tz) {
    if (!SYSCALL_VALIDATE_PTR(tv)) {
        // TODO: Resolve by returning -EFAULT
        syscall_pointerValidateFailed((void*)tv);
    }

    if (tz && !SYSCALL_VALIDATE_PTR(tz)) {
        // TODO: Resolve by returning -EFAULT
        syscall_pointerValidateFailed(tz);
    }

    return settimeofday(tv, tz);
}

/**
 * @brief usleep system call
 */
long sys_usleep(useconds_t usec) {
    if (usec < 10000) {
        // !!!: knock it off, this will crash.
        return 0;
    }

    LOG(DEBUG, "sys_usleep %d\n", usec);
    sleep_untilTime(current_cpu->current_thread, (usec / 10000) / 1000, (usec / 10000) % 1000);
    process_yield(0);

    LOG(DEBUG, "resuming process\n");

    return 0;
}


/**
 * @brief execve system call
 */
long sys_execve(const char *pathname, const char *argv[], const char *envp[]) {
    // Validate pointers
    if (!SYSCALL_VALIDATE_PTR(pathname)) syscall_pointerValidateFailed((void*)pathname);
    if (!SYSCALL_VALIDATE_PTR(argv)) syscall_pointerValidateFailed((void*)argv);
    if (envp && !SYSCALL_VALIDATE_PTR(envp)) syscall_pointerValidateFailed((void*)envp);

    // Try to get the file
    fs_node_t *f = kopen(pathname, O_RDONLY);
    if (!f) return -ENOENT;
    if (f->flags != VFS_FILE) return -EISDIR;

    // Collect any arguments that we need
    int argc = 0;
    int envc = 0;

    // Collect argc
    while (argv[argc]) {
        // TODO: Problem?
        if (!SYSCALL_VALIDATE_PTR(argv[argc])) syscall_pointerValidateFailed((void*)argv[argc]);
        argc++;
    }

    // Collect envc
    if (envp) {
        while (envp[envc]) {
            if (!SYSCALL_VALIDATE_PTR(envp[envc])) syscall_pointerValidateFailed((void*)envp[envc]);
            envc++;
        }
    }

    // Move their arguments into our array
    char *new_argv[argc+1];
    for (int a = 0; a < argc; a++) {
        new_argv[a] = strdup(argv[a]); // TODO: Leaking memory!!!
    }

    // Reallocate envp if specified
    char *new_envp[envc+1];
    if (envp) {
        for (int e = 0; e < envc; e++) {
            new_envp[e] = strdup(envp[e]); // TODO: Leaking memory!!!
        }
    } 

    // Set null terminators
    new_argv[argc] = NULL;
    new_envp[envc] = NULL;

    process_execute(f, argc, new_argv, new_envp);
    return 0;
}