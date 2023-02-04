// tasking.h - contains definitions for the multitasking handler

#ifndef TASKING_H
#define TASKING_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/heap.h" // Heap declarations (memory allocation and such).
#include "include/paging.h" // Paging handler.
#include "include/regs.h" // registers typedefs
#include "include/tss.h" // Task state segment
#include "include/tasking_t.h" // Tasking structure/constants definitions
#include "include/libc/spinlock.h"
#include "include/libc/spinlock_types.h"
#include "include/libc/atomic.h"

// External functions
extern void task_switchContext(size_t **stack); // Switch to current atsk.

// Functions


#endif