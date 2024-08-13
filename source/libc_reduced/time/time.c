// time.c - Provides a replacement for the standard libc file

#include <time.h>

// time(time_t *out) - Returns the time
time_t time(time_t *out) {
    struct timeval p;
    gettimeofday(&p, NULL);
    if (out) {
        *out = p.tv_sec;
    }
    return p.tv_sec;
}

// difftime(time_t a, time_t b) - Returns the difference between two times
double difftime(time_t a, time_t b) {
    return (double)(a - b);
}