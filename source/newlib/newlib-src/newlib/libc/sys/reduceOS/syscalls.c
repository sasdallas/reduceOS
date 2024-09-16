#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include "sys/ioctl.h"
#include "sys/syscall.h"

#include <errno.h>
#undef errno
extern int errno;

void _exit() {
    SYSCALL1(int, SYS_EXIT, 0);
    __builtin_unreachable();
}

int close(int file) { return SYSCALL1(int, SYS_CLOSE, file); }

int execve(char* name, char** argv, char** env) { return SYSCALL3(int, SYS_EXECVE, name, argv, env); }

int fork() { return SYSCALL0(int, SYS_FORK); }

int fstat(int file, struct stat* st) { return SYSCALL2(int, SYS_FSTAT, file, st); }

int getpid() { return SYSCALL0(int, SYS_GETPID); }

int isatty(int file) {
    // No system calls
    int dtype = ioctl(file, 0x4F00, NULL); // 0x4F00 = IOCTLDTYPE
    if (dtype == IOCTL_DTYPE_TTY) return 1;
    errno = EINVAL;
    return 0;
}

int link(char* old, char* new) {
    errno = EMLINK;
    return -1;
}

int lseek(int file, int ptr, int dir) { return SYSCALL3(int, SYS_LSEEK, file, ptr, dir); }

int open(const char* name, int flags, ...) {
    // TODO: va_args?
    int ret = SYSCALL2(int, SYS_OPEN, name, flags);
    if (ret < 0) {
        errno = ret * ret; // newlib uses positive errnos
        return -1;
    }
    return ret;
}

int read(int file, char* ptr, int len) {
    int ret = SYSCALL3(int, SYS_READ, file, ptr, len);
    return ret;
}

int write(int file, char* ptr, int len) {
    int ret = SYSCALL3(int, SYS_WRITE, file, ptr, len);
    return ret;
}

caddr_t sbrk(int incr) {
    uint32_t ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_SBRK), "b"(incr));
    return ret;
}

int stat(const char* __restrict __path, struct stat* __restrict st) { return SYSCALL2(int, SYS_STAT, __path, st); }

clock_t times(struct tms* buf) { return SYSCALL1(unsigned long, SYS_TIMES, buf); }

int unlink(char* name) {
    int ret = SYSCALL1(int, SYS_UNLINK, name);
    if (ret != 0) {
        errno = ret;
        return -1;
    }
    return 0;
}

int wait(int* status) { return SYSCALL1(int, SYS_WAIT, status); }

int mkdir(const char* pathname, mode_t mode) { return SYSCALL2(int, SYS_MKDIR, (char*)pathname, mode); }

int waitpid(pid_t pid, int* status, int options) { return SYSCALL3(int, SYS_WAITPID, pid, status, options); }
