// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// Broken as of 6/30/2023.

#include "include/syscall.h" // Main header file



DECLARE_SYSCALL1(terminalWriteString, 0, const char*);
DECLARE_SYSCALL1(terminalPutchar, 0, char);


// List of system calls
void *syscalls[2] = {
    &terminalWriteString,
    &terminalPutchar

};

uint32_t syscallAmount = 2;



// reduceOS uses interrupt 0x80, and on call of this interrupt, syscallHandler() is called.
// A normal system call would be pretty long for the kernel to call every time, so macros in syscall.h are created.
// These macros declare a function - syscall_<function> - that can automate this process.

// Function prototypes
void syscallHandler();


// initSyscalls() - Registers interrupt handler 0x80 to allow system calls to happen.
void initSyscalls() {
    // As much as I'd rather just call isrRegisterInterruptHandler, we have to do something different.
    // CPU will throw a GPF if IDT DPL < CPL so we have to manually make it use RING3.
    // isrRegisterInterruptHandler(0x80, syscallHandler);
    setVector_flags(0x80, syscallHandler, I86_IDT_DESC_RING3);
    //isrRegisterInterruptHandler(0x80, syscallHandler);
}



void syscallHandler() {
    int syscallNumber;
    asm ("mov %%eax, %0" : "=a"(syscallNumber));

    printf("Syscall received\n");
    printf("Requested for %i\n", syscallNumber);
    // Check if system call number is valid.
    if (syscallNumber >= syscallAmount) {
        return; // Requested call number >= available system calls
    }

    // Get the function.
    void *fn = syscalls[syscallNumber];

    // In this assembly code, we first push all the parameters/return variables, call the function, then pop them.
    int returnValue;
    /*asm volatile("  \
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
    " : "=a" (returnValue) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (fn));*/


    asm volatile ("     \
        push %%edi;      \
        push %%esi;      \
        push %%edx;      \
        push %%ecx;      \
        push %%ebx;      \
        call *%0;        \
        add %%esp, 20;     \
        iret;           \
    " : "=a"(returnValue) : "r"(fn));
}