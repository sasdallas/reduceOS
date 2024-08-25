// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.
// TODO: Better naming conventions, cleaning up code, and fixing an awful definition system.


#include <kernel/syscall.h> // Main header file
#include <kernel/process.h>


// List of system calls
void *syscalls[18] = {
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
    &sys_wait
};

uint32_t syscallAmount = 18;

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

    int syscallNumber = regs->eax;

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
    serialPrintf("%s", buf);
    return nbyte;
}

/*
    Starting from here, the system calls are NOT declared.
    This is because that the above system calls shouldn't be declared in the first place,
    but they are used to test usermode (this codebase I swear)
    We will not declare the below system calls besides in the list.
 */


// SYSCALL 4 
int sys_close(int fd) {
    return -1; // lmao
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
    return 1;
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
    return -1;
}

// SYSCALL 14
// TODO: uint32_t is not the correct type for this, it's arch-specific I believe
uint32_t sys_sbrk(int incr) {
    if (incr & 0xFFF) return -1;
    process_t *proc = currentProcess;
    //if (proc->group != 0) proc = process_from_pid(proc->group);

    if (!proc) return -1;

    spinlock_lock(&proc->image.spinlock);
    uintptr_t out = proc->image.heap;
    serialPrintf("Starting mapping at 0x%x...\n", out);
    for (uintptr_t i = out; i < out + incr; i += 4096) {
        vmm_mapPhysicalAddress(proc->thread.page_directory, i, i, PTE_PRESENT | PTE_WRITABLE | PTE_USER); 
        serialPrintf("Mapping to 0x%x...\n", i);
    }

    serialPrintf("Finished mapping! Sending out 0x%x\n", out);

    proc->image.heap += incr;
    spinlock_release(&proc->image.spinlock);
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
int sys_unlink(char *name) {
    return -1;
}

// SYSCALL 18
int sys_wait(int *status) {
    return -1;
}

