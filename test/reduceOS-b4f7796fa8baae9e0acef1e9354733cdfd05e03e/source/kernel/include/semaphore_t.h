// semaphore_t.h - contains definitions for the semaphore. see semaphore.h for credits.


#ifndef SEMAPHORE_T_H
#define SEMAPHORE_T_H

// Includes
#include "include/libc/stdint.h" // Integer definitions

// Typedefs
typedef struct {
	/// Resource available count
	unsigned int value;
	/// Queue of waiting tasks
	tid_t queue[16];
	/// Position in queue
	unsigned int pos;
	/// Access lock
	spinlock_irqsave_t lock;
} sem_t;

// Macros
#define SEM_INIT(v) {v, {[0 ... 16-1] = 16}, 0, SPINLOCK_IRQSAVE_INIT}



#endif