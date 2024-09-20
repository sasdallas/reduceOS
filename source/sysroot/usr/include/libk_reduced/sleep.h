// sleep.h - header file for sleep.c

#ifndef __INTELLISENSE__
#if !defined __KERNEL__ && !defined __KERNMOD__
#error "This file cannot be used in userspace."
#endif
#endif

#ifndef SLEEP_H
#define SLEEP_H

// Includes
#include <kernel/pit.h> // Programmable interval timer

// Functions
void sleep(int ms); // sleep() - stops execution of current task for x milliseconds.

#endif
