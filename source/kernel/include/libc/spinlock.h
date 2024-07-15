// spinlock.h - header file for the spinlock mechanism

#ifndef SPINLOCK_H
#define SPINLOCK_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include <stdatomic.h> // For once in our lives, C is on our side!!

// Functions
atomic_flag *spinlock_init(); // Creates a new spinlock
void spinlock_lock(atomic_flag *lock); // Locks the spinlock
void spinlock_release(atomic_flag *lock); // Releases the spinlock

#endif