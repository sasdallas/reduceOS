#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H


#include <stddef.h>


// System call list
#define SYS_RESTART_SYSCALL 0
#define SYS_EXIT        1
#define SYS_READ        2
#define SYS_WRITE       3
#define SYS_CLOSE       4
#define SYS_EXECVE      5
#define SYS_FORK        6
#define SYS_FSTAT       7
#define SYS_GETPID      8
#define SYS_ISATTY      9
#define SYS_KILL        10
#define SYS_LINK        11
#define SYS_LSEEK       12
#define SYS_OPEN        13
#define SYS_SBRK        14
#define SYS_STAT        15
#define SYS_TIMES       16
#define SYS_WAIT        17
#define SYS_UNLINK      18
#define SYS_READDIR     19
#define SYS_IOCTL       20
#define SYS_SIGNAL      21
#define SYS_MKDIR       22

// System call macros

#define SYSCALL0(TYPE, NUM) ({ \
    TYPE ret;                   \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (NUM));\
    ret;                        \
})

#define SYSCALL1(TYPE, NUM, ARG0) ({ \
    TYPE ret;                           \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "a" (NUM), "b"(ARG0));\
    ret;                                    \
})


#define SYSCALL2(TYPE, NUM, ARG0, ARG1) ({ \
    TYPE ret;                           \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (NUM), "b"(ARG0), "c"(ARG1));\
    ret;                                    \
})

#define SYSCALL3(TYPE, NUM, ARG0, ARG1, ARG2) ({ \
    TYPE ret;                           \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (NUM), "b"(ARG0), "c"(ARG1), "d"(ARG2));\
    ret;                                    \
})

#define SYSCALL4(TYPE, NUM, ARG0, ARG1, ARG2, ARG3) ({ \
    TYPE ret;                           \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (NUM), "b"(ARG0), "c"(ARG1), "d"(ARG2), \
                    "S"(ARG3));\
    ret;                                    \
})

#define SYSCALL5(TYPE, NUM, ARG0, ARG1, ARG2, ARG3, ARG4) ({ \
    TYPE ret;                           \
    asm volatile ("int $0x80" \
                    : "=a"(ret)\
                    : "0" (NUM), "b"(ARG0), "c"(ARG1), "d"(ARG2), \
                    "S"(ARG3), "D"(ARG4));\
    ret;                                    \
})

#define SYSCALL6(TYPE, NUM, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) ({ \
    TYPE ret;                           \
    int p1p6[2] = {ARG0, ARG5}; \
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
        : "g" (&p1p6), "a"(NUM), "c"(ARG1), "d"(ARG2), \
          "S"(ARG3), "D"(ARG4) : "memory");\
    ret;                                    \
})


#endif