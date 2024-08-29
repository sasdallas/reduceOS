// ctime.c - replacement for the libc file ctime.c

#include <libk_reduced/time.h>

// ctime(const time_t *ptr) - Returns the current time, as a string
char *ctime(const time_t *ptr) {
    return asctime(localtime(ptr));
}
