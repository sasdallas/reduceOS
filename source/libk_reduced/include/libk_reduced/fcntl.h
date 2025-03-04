#ifndef __INTELLISENSE__
#if !defined __KERNEL__ && !defined __KERNMOD__
#error "This file cannot be used in userspace."
#endif
#endif


#include <libk_reduced/stdint.h>
#include <kernel/vfs.h>

#define O_RDONLY     0x0000
#define O_WRONLY     0x0001
#define O_RDWR       0x0002
#define O_APPEND     0x0008
#define O_CREAT      0x0200
#define O_TRUNC      0x0400
#define O_EXCL       0x0800
#define O_NOFOLLOW   0x1000
#define O_PATH       0x2000
#define O_NONBLOCK   0x4000
#define O_DIRECTORY  0x8000

#define F_GETFD 1
#define F_SETFD 2

#define F_GETFL 3
#define F_SETFL 4

#define F_DUPFD 10

/* Advisory locks are not currently supported;
 * these definitions are stubs. */
#define F_GETLK  5
#define F_SETLK  6
#define F_SETLKW 7

#define F_RDLCK  0
#define F_WRLCK  1
#define F_UNLCK  2

struct flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	long l_pid;
};

#define FD_CLOEXEC (1 << 0)
