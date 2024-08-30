// spinlock.h - header file for the spinlock mechanism

#ifndef SPINLOCK_H
#define SPINLOCK_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <stdatomic.h>

typedef atomic_flag spinlock_t;

// Functions
spinlock_t *spinlock_init(); // Creates a new spinlock
void spinlock_lock(spinlock_t *lock); // Locks the spinlock
void spinlock_release(spinlock_t *lock); // Releases the spinlock

#endif
