// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.


#include <kernel/syscall.h> // Main header file
#include <kernel/process.h>

void syscall_sleep();

// List of system calls
void *syscalls[8] = {
    &restart_syscall,
    &_exit,
    &read,
    &write,
    &syscall_fork,
    &execute_process,
    &wait_pid,
    &syscall_sleep
};

uint32_t syscallAmount = 8;

// DECLARATION OF SYSTEM CALLS
DECLARE_SYSCALL0(restart_syscall, 0);
DECLARE_SYSCALL1(_exit, 1, int);
DECLARE_SYSCALL3(read, 2, int, void*, size_t);
DECLARE_SYSCALL3(write, 3, int, const void*, size_t);
DECLARE_SYSCALL0(syscall_fork, 4);
DECLARE_SYSCALL0(execute_process, 5);
DECLARE_SYSCALL0(wait_pid, 6);
DECLARE_SYSCALL0(syscall_sleep, 7);

// END DECLARATION OF SYSTEM CALLS



// reduceOS uses interrupt 0x80, and on call of this interrupt, syscallHandler() is called.
// A normal system call would be pretty long for the kernel to call every time, so macros in syscall.h are created.
// These macros declare a function - syscall_<function> - that can automate this process.

// Function prototypes
void syscallHandler();


// initSyscalls() - Registers interrupt handler 0x80 to allow system calls to happen.
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
long restart_syscall() {
    serialPrintf("restart_syscall: doing things lol\n");
    return 0; // This is supposed to return the system call number being restarted
}

// SYSCALL 1 (https://linux.die.net/man/2/exit)
void _exit(int status) {
    serialPrintf("_exit: Terminating process\n");
    task_exit((status & 0xFF) << 8);
    __builtin_unreachable();
}

// SYSCALL 2 (https://man7.org/linux/man-pages/man2/read.2.html)
// NOTE: Should return a ssize_t but long works I think
long read(int file_desc, void *buf, size_t nbyte) {
    // ssize_t really should be a type though ;)
    serialPrintf("read: system call received for %i bytes on fd %i\n", nbyte, file_desc);
    return nbyte;
}

// SYSCALL 3 (https://man7.org/linux/man-pages/man2/write.2.html)
long write(int file_desc, const void *buf, size_t nbyte) {
    if (!nbyte) return 0;
    serialPrintf("%s", buf);
    return nbyte;
}

pid_t syscall_fork() {
    serialPrintf("Forking process\n");
    return fork();
}


int execute_process() {
    serialPrintf("Starting process 'test_exit'...\n");
    return createProcess("/test_exit");
    for (;;);
}

int wait_pid() {
    return waitpid(-1, NULL, WNOKERN);   
}

void syscall_sleep() {
    // a mimir
    unsigned long s, ss;
    clock_relative(2, 0, &s, &ss);
    sleep_until(currentProcess, s, ss);
    process_switchTask(0);
}