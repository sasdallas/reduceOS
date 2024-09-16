// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// TODO: Better naming conventions, cleaning up code, and fixing an awful definition system.

#include <kernel/clock.h>
#include <kernel/process.h>
#include <kernel/signal.h>
#include <kernel/syscall.h> // Main header file
#include <kernel/vfs.h>
#include <kernel/debug.h>
#include <libk_reduced/errno.h>
#include <libk_reduced/fcntl.h>
#include <libk_reduced/signal.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/time.h>
#include <libk_reduced/stat.h>

// List of system calls
void* syscalls[24] = {
    &sys_restart_syscall, &_exit,      &sys_read,   &sys_write,   &sys_close, &sys_execve, &sys_fork,  &sys_fstat,
    &sys_getpid,          &sys_isatty, &sys_kill,   &sys_link,    &sys_lseek, &sys_open,   &sys_sbrk,  &sys_stat,
    &sys_times,           &sys_wait,   &sys_unlink, &sys_readdir, &sys_ioctl, &sys_signal, &sys_mkdir, &sys_waitpid};

uint32_t syscallAmount = 24;
spinlock_t* write_lock;

// DECLARATION OF TEST SYSTEM CALLS
DECLARE_SYSCALL0(sys_restart_syscall, 0);
DECLARE_SYSCALL1(_exit, 1, int);
DECLARE_SYSCALL3(sys_read, 2, int, void*, size_t);
DECLARE_SYSCALL3(sys_write, 3, int, char*, size_t);
// END DECLARATION OF TEST SYSTEM CALLS

// reduceOS uses interrupt 0x80, and on call of this interrupt, syscallHandler() is called.
// A normal system call would be pretty long for the kernel to call every time, so macros in syscall.h are created.
// These macros declare a function - syscall_<function> - that can automate this process.

// Function prototypes
void syscallHandler();

// initSyscalls() - Registers interrupt handler 0x80 to allow system calls to happen (interrupt marked as usermode in IDT init).
void initSyscalls() {
    isrRegisterInterruptHandler(0x80, syscallHandler);
    write_lock = spinlock_init();
}

void syscallHandler(registers_t* regs) {
    // Set the current process's syscall_registers to the registers here
    currentProcess->syscall_registers = regs;

    uint32_t syscallNumber = regs->eax;

    heavy_dprintf("handle syscall START - num %i...\n", syscallNumber);


    // Check if system call number is valid.
    if (syscallNumber >= syscallAmount) {
        return; // Requested call number >= available system calls
    }

    // Get the function.
    syscall_func* fn = syscalls[syscallNumber];

    int returnValue;

    // Call the system call from the table (TODO: >6 parameter system calls)
    returnValue = fn(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi, regs->ebp);

    heavy_dprintf("handle syscall STOP - ret %i...\n", returnValue);

    // Set EAX to the return value
    asm volatile("mov %0, %%eax" ::"r"(returnValue));
    regs->eax = returnValue;
}

/* SYSTEM CALL HELPERS */

// syscall_validatePointer(void *ptr, const char *syscall) - Validate that a pointer is within range of the program's address space
int syscall_validatePointer(void* ptr, const char* syscall) {
    if (ptr) {
        if (PTR_INRANGE(ptr)) {
            // Let the panic handler take care of it
        } else {
            // Normally we could send a SIGSEGV to the process to say "that aint your memory son" but this is debugging stages.

            panic_prepare();
            disableHardwareInterrupts();
            printf("*** %s: Current process (%s, pid %i) attempted to access memory not accessible to it.\n", syscall,
                   currentProcess->name, currentProcess->id);
            printf("*** The attempted access violation happened at 0x%x\n", ptr);

            serialPrintf("*** %s: Current process (%s, pid %i) attempted to access memory not accessible to it.\n",
                         syscall, currentProcess->name, currentProcess->id);
            serialPrintf("*** The attempted access violation happened at 0x%x\n", ptr);

            panic_dumpPMM();
            registers_t* reg = (registers_t*)((uint8_t*)&end);
            asm volatile("mov %%ebp, %0" ::"r"(reg->ebp));
            reg->eip = NULL; // TODO: Use read_eip()?
            panic_stackTrace(7, reg);

            asm volatile("hlt");
            for (;;);
        }

        pte_t* page = vmm_getPage(ptr);
        if (!page) return 1;
        if (!pte_ispresent(*page) || !pte_iswritable(*page) || (*page & PTE_USER) == PTE_USER) return 1;
    }

    return 0; // Valid
}

/* ACTUAL SYSTEM CALLS */

// SYSCALL 0 (https://man7.org/linux/man-pages/man2/restart_syscall.2.html)
long sys_restart_syscall() {
    serialPrintf("restart_syscall: doing things lol\n");
    return 0; // This is supposed to return the system call number being restarted
}

// SYSCALL 1 (https://linux.die.net/man/2/exit)
// This is the only function that doesn't call have a "sys_" prefix
void _exit(int status) {
    serialPrintf("_exit: Terminating process\n");
    task_exit((status & 0xFF) << 8);
    __builtin_unreachable();
}

// SYSCALL 2 (https://man7.org/linux/man-pages/man2/read.2.html)
// NOTE: Should return a ssize_t but long works I think
long sys_read(int file_desc, void* buf, size_t nbyte) {
    if (!nbyte) return 0;

    if (!SYS_FD_VALIDATE(file_desc)) { return -EBADF; }

    fsNode_t* node = currentProcess->file_descs->nodes[file_desc];
    int64_t out = readFilesystem(node, currentProcess->file_descs->fd_offsets[file_desc], nbyte, (uint8_t*)buf);
    if (out > 0) currentProcess->file_descs->fd_offsets[file_desc] += out;

    return out;
}

// SYSCALL 3 (https://man7.org/linux/man-pages/man2/write.2.html)
long sys_write(int file_desc, char* buf, size_t nbyte) {
    if (!nbyte) return 0;

    if (!SYS_FD_VALIDATE(file_desc)) { return -EBADF; }

    //syscall_validatePointer(buf, "sys_write");

    fsNode_t* node = currentProcess->file_descs->nodes[file_desc];
    int64_t out = writeFilesystem(node, currentProcess->file_descs->fd_offsets[file_desc], nbyte, (uint8_t*)buf);
    if (out > 0) currentProcess->file_descs->fd_offsets[file_desc] += out;
    return out;
}

/*
    Starting from here, the system calls are NOT declared.
    This is because that the above system calls shouldn't be declared in the first place,
    but they are used to test usermode (this codebase I swear)
    We will not declare the below system calls besides in the list.
 */

// SYSCALL 4
int sys_close(int fd) {
    if (currentProcess->file_descs->nodes[fd]) {
        closeFilesystem(currentProcess->file_descs->nodes[fd]);
        currentProcess->file_descs->nodes[fd] = NULL;
        return 0;
    }

    return -EBADF;
}

// SYSCALL 5
int sys_execve(char* name, char** argv, char** env) {
    // serialPrintf("sys_execve: executing %s\n", name);

    bool use_env = ((int)env > 0x1) ? true : false; // BUG: Sometimes env will be 0x1.
    syscall_validatePointer(name, "sys_execve");
    syscall_validatePointer(argv, "sys_execve");
    if (use_env) syscall_validatePointer(env, "sys_execve");

    int argc = 0;
    int envc = 0;

    while ((int)argv[argc] > 0x1) {
        syscall_validatePointer(argv[argc], "sys_execve");
        argc++;
    }

    if (use_env && env) {
        while (env[envc]) { envc++; }
    }

    char** argv_ = kmalloc(sizeof(char*) * (argc + 1));
    for (int j = 0; j < argc; j++) {
        argv_[j] = kmalloc(strlen(argv[j]) + 1);
        memcpy((void*)argv_[j], (void*)argv[j], strlen(argv[j]) + 1);
    }

    argv_[argc] = NULL;

    char** envp;
    if (use_env && env && envc) {
        // Fill it
        envp = kmalloc(sizeof(char*) * (envc + 1));
        for (int j = 0; j < envc; j++) {
            envp[j] = kmalloc(strlen(env[j]) + 1);
            memcpy((void*)envp[j], (void*)env[j], strlen(env[j]) + 1);
        }
    } else {
        envp = kmalloc(sizeof(char*));
        envp[0] = NULL;
    }

    // Close all the file descriptors >2
    for (unsigned int i = 3; i < currentProcess->file_descs->length; i++) {
        if (SYS_FD_VALIDATE(i)) {
            closeFilesystem(currentProcess->file_descs->nodes[i]);
            currentProcess->file_descs->nodes[i] = NULL;
        }
    }

    currentProcess->cmdline = argv_;
    serialPrintf("ready to go, starting execution...\n");

    char* env_empty[] = {NULL};

    return createProcess(name, 1, argv, (use_env) ? envp : env_empty, 1);
}

// SYSCALL 6
int sys_fork(void) { return fork(); }

// SYSCALL 7
int sys_fstat(int file, uintptr_t *st) {
    syscall_validatePointer(st, "sys_fstat");
    if (!st) return -EINVAL; // stupidity

    // Check that the file descriptor exists, return whatever VFS thinks is good.
    if (SYS_FD_VALIDATE(file)) {
        return vfs_statNode(SYS_FD(file), (struct stat *)st);
    }

    return -EBADF;
}

// SYSCALL 8
int sys_getpid(void) { return currentProcess->id; }

// SYSCALL 9
int sys_isatty(int file) {
    SYS_STUB();
    return 1;
}

// SYSCALL 10
int sys_kill(int pid, int sig) {
    if (pid < -1) {
        panic("syscall", "sys_kill", "group_send_signal unimplemented");
    } else if (pid == 0) {
        panic("syscall", "sys_kill", "group_send_signal unimplemented");
    } else {
        serialPrintf("sys_kill: Sending signal %i to pid %i...\n", sig, pid);
        return send_signal(pid, sig, 0);
    }
    __builtin_unreachable();
}

// SYSCALL 11
int sys_link(char* old, char* new) {
    SYS_STUB();
    return 0;
}

// SYSCALL 12
int sys_lseek(int file, int ptr, int dir) {
    // Validate the file descriptor
    if (!SYS_FD_VALIDATE(file)) { return -EBADF; }

    // dir will give us three possible parameters:
    // SEEK_SET     will set the offset of the to ptr bytes
    // SEEK_CUR     will increment the offset by ptr bytes
    // SEEK_END     the offset is set to the end of the file plus ptr bytes

    switch (dir) {
        case SEEK_SET:
            currentProcess->file_descs->fd_offsets[file] = ptr;
            break;
        case SEEK_CUR:
            currentProcess->file_descs->fd_offsets[file] += ptr;
            break;
        case SEEK_END:
            currentProcess->file_descs->fd_offsets[file] = currentProcess->file_descs->nodes[file]->length + ptr;
            break;
        default:
            serialPrintf("sys_lseek: Unknown seek value requested: %i\n", dir);
            return -EINVAL;
    }

    return currentProcess->file_descs->fd_offsets[file];
}

// SYSCALL 13
int sys_open(const char* name, int flags, int mode) {
    fsNode_t* node = open_file(name, flags);

    // sys_open can take a variety of flags, but right now we'll only focus on a few
    // O_CREAT      will create the file if it doesn't exist
    // O_EXCL       will force the call to create the file and fail if it doesn't
    // O_DIRECTORY  will fail the call if the returned object is not a directory

    // Check if we found the file and the call is supposed to fail if we do
    // O_EXCL's behavior is undefined without O_CREAT
    if (node && (flags & O_EXCL) && (flags & O_CREAT)) {
        closeFilesystem(node);
        kfree(node); // VFS does not free yet
        return -EEXIST;
    }

    // If the node does not exist and O_CREAT was specified, try to create the file
    if (!node && (flags & O_CREAT)) {
        int result = createFilesystem((char*)name, mode);
        if (!result) {
            node = open_file((char*)name, flags);
        } else {
            serialPrintf("sys_open: O_CREAT specified but did not succeed\n");
            return result;
        }
    }

    if (node && (flags & O_DIRECTORY)) {
        // User wants directory.
        if (!(node->flags & VFS_DIRECTORY)) {
            // Not chill
            closeFilesystem(node);
            kfree(node);
            return -ENOTDIR;
        }
    }

    if (!node) {
        // Not found
        return -ENOENT;
    }

    // O_CREAT and directories don't mix
    if (node && (flags & O_CREAT) && (node->flags & VFS_DIRECTORY)) {
        closeFilesystem(node);
        kfree(node);
        return -EISDIR;
    }

    int fd = process_addfd(currentProcess, node);

    if (flags & O_APPEND) {
        currentProcess->file_descs->fd_offsets[fd] = node->length;
    } else {
        currentProcess->file_descs->fd_offsets[fd] = 0;
    }

    return fd;
}

// SYSCALL 14
// TODO: uint32_t is not the correct type for this, it's arch-specific I believe
uint32_t sys_sbrk(int incr) {
    process_t* proc = currentProcess;

    if (!proc) return -1;

    spinlock_lock(&proc->image.spinlock);

    uintptr_t out = proc->image.heap; // Old heap value


    // Make sure something isn't going on
    if (proc->image.heap + incr < proc->image.heap_start) {
        panic("reduceOS", "DEBUG SYSCALL", "The process' heap expanded to below its start.");
    }

    proc->image.heap += incr;
    proc->image.heap_end += incr;

    spinlock_release(&proc->image.spinlock);
    serialPrintf("sys_sbrk: Heap done - new value: 0x%x - 0x%x\n", proc->image.heap_start, (uint32_t)out + incr);
    return (uint32_t)out;
}

// SYSCALL 15
int sys_stat(char* file, uintptr_t *st) {
    syscall_validatePointer(st, "sys_fstat");
    syscall_validatePointer(file, "sys_fstat");

    if (!st || !file) return -EINVAL; // stupidity

    // Open the file, stat it, and return
    fsNode_t *node = open_file(file, 0);
    int result = vfs_statNode(node, (struct stat *)st);
    if (node) {
        closeFilesystem(node);
    }

    return result;
}

// SYSCALL 16
unsigned long sys_times(struct tms* buf) {
    if (buf) {
        syscall_validatePointer(buf, "sys_times");

        buf->tms_utime = (currentProcess->time_total - currentProcess->time_sys) / clock_getTSCSpeed();
        buf->tms_stime = currentProcess->time_sys / clock_getTSCSpeed();
        buf->tms_cutime = (currentProcess->time_children - currentProcess->time_sys_children) / clock_getTSCSpeed();
        buf->tms_cstime = currentProcess->time_sys_children / clock_getTSCSpeed();
    }

    return clock_getTimer() / clock_getTSCSpeed();
}

// SYSCALL 17
int sys_wait(int* status) { return waitpid(-1, status, WNOKERN); }

// SYSCALL 18
int sys_unlink(char* name) {
    SYS_STUB();
    return -ENOENT;
}

// SYSCALL 19
int sys_readdir(int fd, int cur_entry, struct dirent* entry) {
    // Check the file descriptor
    if (SYS_FD_VALIDATE(fd)) {
        syscall_validatePointer(entry, "sys_readdir");

        if (!entry) return -EINVAL; // not the correct error code
        struct dirent* kentry = readDirectoryFilesystem(
            SYS_FD(fd), (uint32_t)cur_entry); // TODO: readdirFilesystem takes in a uint32_t, probably not good

        if (kentry) {
            memcpy(entry, kentry, sizeof *entry);
            kfree(entry);
            return 1;
        } else {
            return 0;
        }
    }

    return -EBADF;
}

// SYSCALL 20
int sys_ioctl(int fd, unsigned long request, void* argp) {
    if (SYS_FD_VALIDATE(fd)) {
        if (argp) syscall_validatePointer(argp, "sys_ioctl"); // argp can be 0

        return ioctlFilesystem(SYS_FD(fd), request, argp);
    }

    return -EBADF;
}

// SYSCALL 21
long sys_signal(long signum, uintptr_t handler) {
    serialPrintf("sys_signal: Trying to register handler for signum %i handler 0x%x\n", signum, handler);

    if (signum >= NUMSIGNALS || signum < 0) return -1;
    if (signum == SIGKILL || signum == SIGSTOP) return -1;

    // signal() wants us to return the old handler
    uintptr_t old_handler = currentProcess->signals[signum].handler;

    // Set the new handler
    currentProcess->signals[signum].handler = handler;
    currentProcess->signals[signum].flags = SA_RESTART;

    serialPrintf("sys_signal: Handler is all setup.\n");

    return old_handler;
}

// SYSCALL 22
int sys_mkdir(char* pathname, int mode) {
    // TODO: Replace this with mode_t
    // TODO: Even use the mode variable

    syscall_validatePointer(pathname, "sys_mkdir");
    if (!pathname) return -EINVAL;

    // For some reason, a call will be made to create a directory with the current .
    if (!strcmp(pathname, ".")) return 0;

    return mkdirFilesystem(pathname, (uint16_t)mode);
}

// SYSCALL 23
int sys_waitpid(pid_t pid, int* status, int options) { return waitpid(pid, status, options); }
