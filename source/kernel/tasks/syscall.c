// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// Broken as of 6/30/2023.

#include "include/syscall.h" // Main header file



DECLARE_SYSCALL1(terminalWriteString, 0, const char*);

// List of system calls
void *syscalls[] = {
    &terminalWriteString
};

uint32_t syscallAmount = 1;



// reduceOS uses interrupt 0x80, and on call of this interrupt, syscallHandler() is called.
// A normal system call would be pretty long for the kernel to call every time, so macros in syscall.h are created.
// These macros declare a function - syscall_<function> - that can automate this process.

// Function prototypes
void syscallHandler(registers_t *regs);


// initSyscalls() - Registers interrupt handler 0x80 to allow system calls to happen.
void initSyscalls() {
    // Register interrupt handler 0x80
    isrRegisterInterruptHandler(0x80, syscallHandler);
}



void syscallHandler(registers_t *regs) {
    // Check if system call number is valid.
    if (regs->eax >= syscallAmount) {
        return; // Requested call number >= available system calls
    }

    // Get the function.
    void *fn = syscalls[regs->eax];

    // In this assembly code, we first push all the parameters/return variables, call the function, then pop them.
    int returnValue;
    asm volatile("  \
        push %1;     \
        push %2;     \
        push %3;     \
        push %4;     \
        push %5;     \
        call *%6;    \
        pop %%ebx;   \
        pop %%ebx;   \
        pop %%ebx;   \
        pop %%ebx;   \
        pop %%ebx;   \  
    " : "=a" (returnValue) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (fn));
    

    regs->eax = returnValue; // Set EAX to the returnValue.  
}