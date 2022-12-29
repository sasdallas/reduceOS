// sleep.c - Custom libc header file that uses the programmable interval timer to sleep for X milliseconds.


#include "include/libc/sleep.h" // Main header file


void sleep(int ms) {
    int currentTickCount = i86_pitGetTickCount();
    while (i86_pitGetTickCount() - currentTickCount < ms);
}