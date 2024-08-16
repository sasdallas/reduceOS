// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.
// Note that while I try my best to adhere to C standards, none of the libc function for the kernel will ever be used by an application, it's to make the code more easier to read.

#include <sleep.h> // Main header file
#include <time.h>

void sleep(int ms) {
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t end_ticks = t.tv_usec + (ms * 1000);

    // Wait until we reach the target
    do {
        gettimeofday(&t, NULL);
    } while (t.tv_usec < end_ticks);
}