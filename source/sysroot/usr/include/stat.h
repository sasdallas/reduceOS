#ifndef STAT_H
#define STAT_H

#include <stdint.h>
#include <time.h>

struct stat 
{
	uint16_t		st_dev;		/* ID of device containing file */
	uint16_t		st_ino;		/* inode number */
	uint32_t		st_mode;	/* protection */
	unsigned short	st_nlink;	/* number of hard links */
	uint16_t		st_uid;		/* user ID of owner */
	uint16_t		st_gid;		/* group ID of owner */
	uint16_t		st_rdev;	/* device ID (if special file) */
	off_t		    st_size;	/* total size, in bytes */
	time_t          st_atim;	/* time of last access */
	time_t          st_mtim;	/* time of last modification */
	time_t          st_ctim;	/* time of last status change */
	uint32_t	st_blksize;	/* blocksize for filesystem I/O */
	uint32_t	st_blocks;	/* number of blocks allocated */
} stat_t;

#endif 