// syscall.h - handler for system calls

#ifndef SYSCALL_H
#define SYSCALL_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/isr.h" // Interrupt Service Routines

// Macros
// See syscall.c for an explanation of why we need these macros.

// 5 DEFINE_SYSCALL macros (just function prototypes)
#define DEFINE_SYSCALL0(func) int syscall_##func();
#define DEFINE_SYSCALL1(func, p1) int syscall_##func(p1);

// Define some system calls
DEFINE_SYSCALL1(terminalWriteString, const char*);
DEFINE_SYSCALL1(terminalPutchar, char);
DEFINE_SYSCALL0(terminalUpdateScreen);

// 5 DECLARE_SYSCALL macros, the only reason we have 5 is to allow for parameters.
// EAX = Number, EBX = Parameter 1, ECX = Parameter 2, EDX = Parameter 3

#define DECLARE_SYSCALL0(func, num) \
int syscall_##func() { \
    int a; \
    asm volatile ("int $0x80" : "=a" (a) : "0" (num)); \
    return a; \
} 

#define DECLARE_SYSCALL1(func, num, P1) \
int syscall_##func(P1 p1) { \
    int a; \
    asm volatile ("int $0x80" : "=a" (a) : "0" (num), "b"((int)p1)); \
    return a; \
} 


// Functions
void initSyscalls();


#endif