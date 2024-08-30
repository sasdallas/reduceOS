#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <stdio.h>
#include "sys/syscall.h"

#include <errno.h>
#undef errno
extern int errno;

register char * stack_ptr asm ("esp");

// Environmental variables
char *__env[1] = { 0 };
char **environ = __env;

void _exit() {
    SYSCALL1(int, SYS_EXIT, 0);
    __builtin_unreachable();
}

int close(int file) {
    return -1;
}

int execve(char *name, char **argv, char **env) {
    int ret = SYSCALL3(int, SYS_EXECVE, name, argv, env);
    return -1;
}

int fork() {
    int ret = SYSCALL0(int, SYS_FORK);
    return ret;
}

int fstat(int file, struct stat *st) {
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

int kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int link(char *old, char *new) {
    errno = EMLINK;
    return -1;
}

int lseek(int file, int ptr, int dir) {
    return 0;
}

int open(const char *name, int flags, ...) {
    return -1;
}

int read(int file, char *ptr, int len) {
    return 0;
}


int write(int file, char *ptr, int len) {
    int ret = SYSCALL3(int, SYS_WRITE, file, ptr, len);
    return ret;
}



caddr_t sbrk(int incr) {
    uint32_t ret;
    asm volatile ("int $0x80" : "=a"(ret) : "a"(SYS_SBRK), "b"(incr));
    if (ret < 0x1000) {
        write(3, "newlib: SBRK error!", strlen("newlib: SBRK error!"));
        abort();    
    }
    
    return ret;
}


int stat(const char *__restrict __path, struct stat *__restrict st) {
    st->st_mode = S_IFCHR;
    return 0;
}

clock_t times(struct tms *buf) {
    return  -1;
}

int unlink(char *name) {
    errno = ENOENT;
    return -1;
}


int wait(int *status) {
    errno = ECHILD;
    return -1;
}
