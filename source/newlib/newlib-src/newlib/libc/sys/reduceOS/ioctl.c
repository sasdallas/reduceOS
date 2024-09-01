// ioctl.c - handles I/O control system call

#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <errno.h>

#ifdef errno
#undef errno
extern int errno;
#endif


int ioctl(int fd, int request, void *argp) {
    return SYSCALL3(int, SYS_IOCTL, fd, request, argp);
}