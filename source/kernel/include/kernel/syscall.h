// syscall.h - handler for system calls

#ifndef SYSCALL_H
#define SYSCALL_H

// Includes
#include <stdint.h> // Integer declarations
#include <kernel/isr.h> // Interrupt Service Routines

// Macros
// See syscall.c for an explanation of why we need these macros.

// 5 DEFINE_SYSCALL macros (just function prototypes)
#define DEFINE_SYSCALL0(func) int syscall_##func();
#define DEFINE_SYSCALL1(func, p1) int syscall_##func(p1);
#define DEFINE_SYSCALL2(func, p1, p2) int syscall_##func(p1, p2);
#define DEFINE_SYSCALL3(func, p1, p2, p3) int syscall_##func(p1, p2, p3);
#define DEFINE_SYSCALL4(func, p1, p2, p3, p4) int syscall_##func(p1, p2, p3, p4);
#define DEFINE_SYSCALL5(func, p1, p2, p3, p4, p5) int syscall_##func(p1, p2, p3, p4, p5);
#define DEFINE_SYSCALL6(func, p1, p2, p3, p4, p5, p6) int syscall_##func(p1, p2, p3, p4, p5, p6);




// 6 macros to declare syscall functions
// EAX = Number, EBX = Parameter 1, ECX = Parameter 2, EDX = Parameter 3
// ESI = Parameter 4, EDI = Parameter 5, EBP = Parameter 6

#define DECLARE_SYSCALL0(func, num) \
int syscall_##func() { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num)); \
    return ret; \
} 

#define DECLARE_SYSCALL1(func, num, P1) \
int syscall_##func(P1 p1) { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num), "b"(p1)); \
    return ret; \
} 

#define DECLARE_SYSCALL2(func, num, P1, P2) \
int syscall_##func(P1 p1, P2 p2) { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num), "b"(p1), "c"(p2)); \
    return ret; \
} 

#define DECLARE_SYSCALL3(func, num, P1, P2, P3) \
int syscall_##func(P1 p1, P2 p2, P3 p3) { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num), "b"(p1), "c"(p2), "d"(p3)); \
    return ret; \
} 

#define DECLARE_SYSCALL4(func, num, P1, P2, P3, P4) \
int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4) { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num), "b"(p1), "c"(p2), "d"(p3), \
                        "S"(p4)); \
    return ret; \
} 

#define DECLARE_SYSCALL5(func, num, P1, P2, P3, P4, P5) \
int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) { \
    int ret; \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (num), "b"(p1), "c"(p2), "d"(p3), \
                        "S"(p4), "D"(p5)); \
    return ret; \
} 

#define DECLARE_SYSCALL6(func, num, P1, P2, P3, P4, P5, P6) \
int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) { \
    int ret; \
    int p1p6[2] = {p1, p6}; \
    asm volatile ("pushl %1;"   \           
        "push %%ebx;"           \
        "push %%ebp;"           \
        "mov 8(%%esp), %%ebx;"  \
        "mov 4(%%ebx), %%ebp;"  \
        "mov (%%ebx), %%ebx;"   \
        "int $0x80;"            \
        "pop %%ebp;"            \
        "pop %%ebx;"            \
        "add $4, %%esp;"        \
        : "=a"(ret)     \
        : "g" (&p1p6), "a"(num), "c"(p2), "d"(p3), \
          "S"(p4), "D"(p5) : "memory"); \
    return ret; \
}


// Typedefs
typedef int syscall_func(int p1, int p2, int p3, int p4, int p5, int p6);



// Functions
void initSyscalls();

// Syscall prototypes
long restart_syscall();
void _exit(int status);
long read(int file_desc, void *buf, size_t nbyte);
long write(int file_desc, const void *buf, size_t nbyte);

// Syscall definitions
DEFINE_SYSCALL0(restart_syscall);
DEFINE_SYSCALL1(_exit, int);
DEFINE_SYSCALL3(read, int, void*, size_t);
DEFINE_SYSCALL3(write, int, const void*, size_t);

#endif
