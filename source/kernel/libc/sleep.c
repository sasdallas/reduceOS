// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.


#include "include/libc/sleep.h" // Main header file


void sleep(int ms) {
    // Originally, the default for a PIT timer is around 18.222Hz.
    // Luckily, our i86_pitInit function intializes the PIT with 100Hz.

    int startTickCount = i86_pitGetTickCount();
    while (i86_pitGetTickCount() - startTickCount < ms);
    return;
}