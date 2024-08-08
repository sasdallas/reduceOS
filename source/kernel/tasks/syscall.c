// ====================================================
// syscall.c - handles reduceOS system call interface
// ====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.


#include <kernel/syscall.h> // Main header file



DECLARE_SYSCALL0(syscall0, 0);
DECLARE_SYSCALL1(syscall1, 1, int);
DECLARE_SYSCALL2(syscall2, 2, int, int);
DECLARE_SYSCALL3(syscall3, 3, int, int, int);
DECLARE_SYSCALL4(syscall4, 4, int, int, int, int);
DECLARE_SYSCALL5(syscall5, 5, int, int, int, int, int);
DECLARE_SYSCALL6(syscall6, 6, int, int, int, int, int, int);

// List of system calls (TODO: This list sucks)
void *syscalls[7] = {
    &syscall0,
    &syscall1,
    &syscall2,
    &syscall3,
    &syscall4,
    &syscall5,
    &syscall6
};

uint32_t syscallAmount = 7; // This sucks too




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
    int syscallNumber = regs->eax;

    // Check if system call number is valid.
    if (syscallNumber >= syscallAmount) {
        return; // Requested call number >= available system calls
    }

    // Get the function.
    syscall_func *fn = syscalls[syscallNumber];
   
    // In this assembly code, we first push all the parameters/return variables, call the function, then pop them.
    int returnValue;

    // Call the system call from the table (TODO: >6 parameter system calls)
    fn(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi, regs->ebp);
}
