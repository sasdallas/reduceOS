// syscall.h - handler for system calls

#ifndef SYSCALL_H
#define SYSCALL_H

// Includes
#include <kernel/isr.h> // Interrupt Service Routines
#include <kernel/process.h>
#include <libk_reduced/stdint.h> // Integer declarations

// Macros
// See syscall.c for an explanation of why we need these macros.

// Validating userspace pointers
// In this situation, even though the heap is not mapped, the page fault handler will take care of it.
#define PTR_INRANGE(PTR)                                                                                               \
    (((uintptr_t)PTR > currentProcess->image.entrypoint)                                                               \
     || ((uintptr_t)PTR > currentProcess->image.heap_start && (uintptr_t)PTR < currentProcess->image.heap_end))

// File descriptor validation
#define SYS_FD_VALIDATE(fd)                                                                                            \
    (fd < currentProcess->file_descs->length && fd >= 0 && currentProcess->file_descs->nodes[fd])
#define SYS_FD(fd)                                    (currentProcess->file_descs->nodes[fd])

// Stub system calls
#define SYS_STUB()                                    (serialPrintf("%s: Stub system call.\n", __FUNCTION__))

// 5 DEFINE_SYSCALL macros (just function prototypes)
#define DEFINE_SYSCALL0(func)                         int syscall_##func();
#define DEFINE_SYSCALL1(func, p1)                     int syscall_##func(p1);
#define DEFINE_SYSCALL2(func, p1, p2)                 int syscall_##func(p1, p2);
#define DEFINE_SYSCALL3(func, p1, p2, p3)             int syscall_##func(p1, p2, p3);
#define DEFINE_SYSCALL4(func, p1, p2, p3, p4)         int syscall_##func(p1, p2, p3, p4);
#define DEFINE_SYSCALL5(func, p1, p2, p3, p4, p5)     int syscall_##func(p1, p2, p3, p4, p5);
#define DEFINE_SYSCALL6(func, p1, p2, p3, p4, p5, p6) int syscall_##func(p1, p2, p3, p4, p5, p6);

// 6 macros to declare syscall functions
// EAX = Number, EBX = Parameter 1, ECX = Parameter 2, EDX = Parameter 3
// ESI = Parameter 4, EDI = Parameter 5, EBP = Parameter 6

#define DECLARE_SYSCALL0(func, num)                                                                                    \
    int syscall_##func() {                                                                                             \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num));                                                              \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL1(func, num, P1)                                                                                \
    int syscall_##func(P1 p1) {                                                                                        \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(p1));                                                     \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL2(func, num, P1, P2)                                                                            \
    int syscall_##func(P1 p1, P2 p2) {                                                                                 \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(p1), "c"(p2));                                            \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL3(func, num, P1, P2, P3)                                                                        \
    int syscall_##func(P1 p1, P2 p2, P3 p3) {                                                                          \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(p1), "c"(p2), "d"(p3));                                   \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL4(func, num, P1, P2, P3, P4)                                                                    \
    int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4) {                                                                   \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(p1), "c"(p2), "d"(p3), "S"(p4));                          \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL5(func, num, P1, P2, P3, P4, P5)                                                                \
    int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {                                                            \
        int ret;                                                                                                       \
        asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(p1), "c"(p2), "d"(p3), "S"(p4), "D"(p5));                 \
        return ret;                                                                                                    \
    }

#define DECLARE_SYSCALL6(func, num, P1, P2, P3, P4, P5, P6)                                                            \
    int syscall_##func(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {                                                     \
        int ret;                                                                                                       \
        int p1p6[2] = {p1, p6};                                                                                        \
        asm volatile("pushl %1;"   \           
        "push %%ebx;"                                                                                                  \
        "push %%ebp;"                                                                                                  \
        "mov 8(%%esp), %%ebx;"                                                                                         \
        "mov 4(%%ebx), %%ebp;"                                                                                         \
        "mov (%%ebx), %%ebx;"                                                                                          \
        "int $0x80;"                                                                                                   \
        "pop %%ebp;"                                                                                                   \
        "pop %%ebx;"                                                                                                   \
        "add $4, %%esp;"                                                                                               \
                     : "=a"(ret)                                                                                       \
                     : "g"(&p1p6), "a"(num), "c"(p2), "d"(p3), "S"(p4), "D"(p5)                                        \
                     : "memory");                                                                                      \
        return ret;                                                                                                    \
    }

// Typedefs
typedef int syscall_func(int p1, int p2, int p3, int p4, int p5, int p6);

// Functions
void initSyscalls();
int syscall_validatePointer(void* ptr, const char* syscall);

// Syscall prototypes
long sys_restart_syscall();
void _exit(int status);
long sys_read(int file_desc, void* buf, size_t nbyte);
long sys_write(int file_desc, char* buf, size_t nbyte);
int sys_close(int fd);
int sys_execve(char* name, char** argv, char** env);
int sys_fork(void);
int sys_fstat(int file, void* st);
int sys_getpid(void);
int sys_isatty(int file);
int sys_kill(int pid, int sig);
int sys_link(char* old, char* new);
int sys_lseek(int file, int ptr, int dir);
int sys_open(const char* name, int flags, int mode);
uint32_t sys_sbrk(uint32_t incr_uint);
int sys_stat(char* file, void* st);
int sys_times(void* buf);
int sys_unlink(char* name);
int sys_wait(int* status);
int sys_readdir(int fd, int cur_entry, struct dirent* entry);
int sys_ioctl(int fd, unsigned long request, void* argp);
long sys_signal(long signum, uintptr_t handler);
void syscallHandler(registers_t* regs);
int sys_mkdir(char* pathname, int mode);
int sys_waitpid(pid_t pid, int* status, int options);

// Syscall definitions (only test system calls)
DEFINE_SYSCALL0(sys_restart_syscall);
DEFINE_SYSCALL1(_exit, int);
DEFINE_SYSCALL3(sys_read, int, void*, size_t);
DEFINE_SYSCALL3(sys_write, int, char*, size_t);

#endif
