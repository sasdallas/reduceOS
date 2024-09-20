#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#define IOCTLDTYPE 0x4F00

#define IOCTL_DTYPE_UNKNOWN -1
#define IOCTL_DTYPE_FILE     1
#define IOCTL_DTYPE_TTY      2

#define IOCTLTTYNAME  0x4F01
#define IOCTLTTYLOGIN 0x4F02
#define IOCTLSYNC     0x4F03

#define IOCTL_PACKETFS_QUEUED 0x5050

#define FIONBIO  0x4e424c4b

int ioctl(int fd,int request, void *argp);

#endif