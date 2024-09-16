// Did not write this file - sourced from Linux newlib sources.

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/dirent.h>

#include <errno.h>
#undef errno
extern int errno;

DIR *opendir(const char *dirname) {
    int fd = open(dirname, O_RDONLY);
    if (fd < 0) {
        return NULL; // open() sets an errno, don't overwrite it
    }

    // Got the directory, allocate a structure and copy the values
    DIR *dir = (DIR*)malloc(sizeof(DIR));
    dir->fd = fd;
    dir->cur_entry = -1;
    return dir;
}

int closedir(DIR *dir) {
    if (dir && (dir->fd != -1)) {
        return close(dir->fd);
    } else {
        return -EBADF;
    }
}

struct dirent *readdir(DIR *dirp) {
    static struct dirent ent;
    int ret = SYSCALL3(int, SYS_READDIR, dirp->fd, ++dirp->cur_entry, &ent);
    if (ret < 0) {
        errno = -ret;
        memset(&ent, 0, sizeof(struct dirent));
        return NULL;
    }

    if (ret == 0) {
        memset(&ent, 0, sizeof(struct dirent));
        return NULL;
    }

    return &ent;
}