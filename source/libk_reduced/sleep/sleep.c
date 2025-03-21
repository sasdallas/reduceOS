// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.
// Note that while I try my best to adhere to C standards, none of the libc function for the kernel will ever be used by an application, it's to make the code more easier to read.

#include <libk_reduced/sleep.h> // Main header file
#include <libk_reduced/time.h>

void sleep(int ms) {
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t end_ticks = (t.tv_sec * 1000 * 1000) + t.tv_usec + (ms * 1000);

    // Wait until we reach the target
    do {
        //serialPrintf("SLEEPING - %016llX vs %016llX\n", (t.tv_sec * 1000 * 1000) + t.tv_usec, end_ticks);
        gettimeofday(&t, NULL);
    } while (((t.tv_sec * 1000 * 1000) + t.tv_usec) < end_ticks);
}
