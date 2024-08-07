// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.
// Note that while I try my best to adhere to C standards, none of the libc function for the kernel will ever be used by an application, it's to make the code more easier to read.

#include <sleep.h> // Main header file


void sleep(int ms) {
    // Originally, the default for a PIT timer is around 18.222Hz.
    // Luckily, our pitInit function intializes the PIT with 100Hz.

    int startTickCount = pitGetTickCount();
    while (pitGetTickCount() - startTickCount < ms);
    return;
}