// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.


#include "include/libc/sleep.h" // Main header file


void sleep(int ms) {
    int originalTickCount = i86_pitGetTickCount(); // Save the original tick count
    int currentTickCount;

    while (i86_pitGetTickCount() - originalTickCount != ms);
    return;
}