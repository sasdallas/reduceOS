// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// TODO: Better naming conventions, cleaning up code, and fixing an awful definition system.


#include <kernel/syscall.h> // Main header file
#include <kernel/process.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/fcntl.h>
#include <libk_reduced/errno.h>


// List of system calls
void *syscalls[19] = {
    &sys_restart_syscall,
    &_exit,
    &sys_read,
    &sys_write,
    &sys_close,
    &sys_execve,
    &sys_fork,
    &sys_fstat,
    &sys_getpid,
    &sys_isatty,
    &sys_kill,
    &sys_link,
    &sys_lseek,
    &sys_open,
    &sys_sbrk,
    &sys_stat,
    &sys_times,
    &sys_wait,
    &sys_unlink
};

uint32_t syscallAmount = 19;
spinlock_t *write_lock;

// DECLARATION OF TESTSYSTEM CALLS
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


void syscallHandler(registers_t *regs) {
    // Set the current process's syscall_registers to the registers here
    if (!currentProcess->syscall_registers) {
        // TODO: Not do this. Just set currentProcess->syscall_registers to regs. This is not freed. Need to remove.
        currentProcess->syscall_registers = kmalloc(sizeof(registers_t));
        memcpy(currentProcess->syscall_registers, regs, sizeof(registers_t));
    } else {
        memcpy(currentProcess->syscall_registers, regs, sizeof(registers_t));
    }

    uint32_t syscallNumber = regs->eax;

    // Check if system call number is valid.
    if (syscallNumber >= syscallAmount) {
        return; // Requested call number >= available system calls
    }

    // Get the function.
    syscall_func *fn = syscalls[syscallNumber];
    
    int returnValue;

    // Call the system call from the table (TODO: >6 parameter system calls)
    returnValue = fn(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi, regs->ebp);

    // Set EAX to the return value
    asm volatile ("mov %0, %%eax" :: "r"(returnValue));
    regs->eax = returnValue;
    currentProcess->syscall_registers->eax = returnValue; // TODO: Need this?
}


/* SYSTEM CALL HELPERS */

// syscall_validatePointer(void *ptr, const char *syscall) - Validate that a pointer is within range of the program's address space
int syscall_validatePointer(void *ptr, const char *syscall) {
    if (ptr) {
        if (!PTR_INRANGE(ptr)) { 
            // Normally we could send a SIGSEGV to the process to say "that aint your memory son" but we don't have that.
            // So just crash! I'm sure nothing could go wrong!
            panic("syscall", "pointer vaildation", "Current process attempted to access memory outside of its address space.");
        }

        pte_t *page = vmm_getPage(ptr);
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
long sys_read(int file_desc, void *buf, size_t nbyte) {
    // ssize_t really should be a type though ;)
    serialPrintf("read: system call received for %i bytes on fd %i\n", nbyte, file_desc);
    return nbyte;
}


// SYSCALL 3 (https://man7.org/linux/man-pages/man2/write.2.html)
long sys_write(int file_desc, char *buf, size_t nbyte) {
    if (!nbyte) return 0;

    if (file_desc == 2) {
        // stderr, force it
        serialPrintf("%s", buf);
        return nbyte;
    }

    if (!currentProcess->file_descs->nodes[file_desc]) {
        return 0;
    }


    //syscall_validatePointer(buf, "sys_write");

    fsNode_t *node = currentProcess->file_descs->nodes[file_desc];
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
int sys_execve(char *name, char **argv, char **env) {
    return -1;
}

// SYSCALL 6
int sys_fork(void) {
    return fork();
}

// SYSCALL 7
int sys_fstat(int file, void *st) {
    return -1;
}

// SYSCALL 8
int sys_getpid(void) {
    return currentProcess->id;
}

// SYSCALL 9
int sys_isatty(int file) {
    return 1;
}

// SYSCALL 10
int sys_kill(int pid, int sig) {
    return -1;
}

// SYSCALL 11
int sys_link(char *old, char *new) {
    return -1;
}

// SYSCALL 12
int sys_lseek(int file, int ptr, int dir) {
    return 0;
}

// SYSCALL 13
int sys_open(const char *name, int flags, int mode) {
    fsNode_t *node = open_file(name, flags);

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
            node =  open_file((char*)name, flags);
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
uint32_t sys_sbrk(uint32_t incr_uint) {
    int incr = (int)incr_uint;
    process_t *proc = currentProcess;

    if (!proc) return -1;

    spinlock_lock(&proc->image.spinlock);
    

    uintptr_t out = proc->image.heap; // Old heap value

    if (incr < 0) {
        serialPrintf("sys_sbrk: WARNING! Support for negative SBRK is not implemented yet!\n");
    
        serialPrintf("sys_sbrk: If we were to do such a thing though, the heap would be set to %i\n", out + incr);
        goto end;
    }


    /* 
        Here is how the reduceOS sbrk() handler works:
            1. Increase the heap pointer and return the old one
            2. The program will attempt to access invalid and unmapped memory
            3. This is stupid but hilarious - the page fault handler recognizes this, quietly maps the address, then slinks away.
    */
    proc->image.heap += incr;
    proc->image.heap_end = proc->image.heap;
    ASSERT(proc->image.heap_start, "sys_sbrk", "Current process does not have an available heap_start");

end:
    spinlock_release(&proc->image.spinlock);
    serialPrintf("sys_sbrk: Heap done - returning 0x%x\n", (uint32_t)out);
    return (uint32_t)out;
}

// SYSCALL 15
int sys_stat(char *file, void *st) {
    return 0; // Not implemented.
}

// SYSCALL 16
int sys_times(void *buf) {
    return -1;
}

// SYSCALL 17
int sys_wait(int *status) {
    return waitpid(-1, status, 0);
}

// SYSCALL 18
int sys_unlink(char *name) {
    return -ENOENT;
}

