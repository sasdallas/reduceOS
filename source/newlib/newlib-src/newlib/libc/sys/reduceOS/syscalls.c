#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include "sys/syscall.h"

#include <errno.h>
#undef errno
extern int errno;

register char* stack_ptr asm("esp");

void _exit() {
    SYSCALL1(int, SYS_EXIT, 0);
    __builtin_unreachable();
}

int close(int file) { return -1; }

int execve(char* name, char** argv, char** env) {
    int ret = SYSCALL3(int, SYS_EXECVE, name, argv, env);
    return ret;
}

int fork() {
    int ret = SYSCALL0(int, SYS_FORK);
    return ret;
}

int fstat(int file, struct stat* st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int getpid() {
    int ret = SYSCALL0(int, SYS_GETPID);
    return ret;
}

int isatty(int file) {
    int ret = SYSCALL0(int, SYS_ISATTY);
    return ret;
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
    if (ret < 0x1000) {
        write(2, "newlib: SBRK error!", strlen("newlib: SBRK error!"));
        abort();
    }

    return ret;
}

int stat(const char* __restrict __path, struct stat* __restrict st) { return SYSCALL2(int, SYS_STAT, __path, st); }

clock_t times(struct tms* buf) { return SYSCALL1(clock_t, SYS_TIMES, buf); }

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
