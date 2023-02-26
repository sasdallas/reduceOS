// spinlock_types.h - contains the definition for a few structures, such as spinlock_t
// This implementation of spinlock is based of eduOS by RWTH-OS.

#ifndef SPINLOCK_TYPES_H
#define SPINLOCK_TYPES_H

// Includes
#include "include/libc/stdint.h" // Integer definitions.
#include "include/libc/atomic.h"
#include "include/tasking_t.h"

// Typedefs
typedef struct spinlock {
	/// Internal queue
	atomic_int32_t queue;
	/// Internal dequeue 
	atomic_int32_t dequeue;
	/// Owner of this spinlock structure
	uint32_t owner;
	/// Internal counter var
	uint32_t counter;
} spinlock_t;

typedef struct spinlock_irqsave {
	/// Internal queue
	atomic_int32_t queue;
	/// Internal dequeue
	atomic_int32_t dequeue;
	/// Internal counter var
	uint32_t counter;
	/// Interrupt flag
	uint8_t flags;
} spinlock_irqsave_t;


/// Macro for spinlock initialization
#define SPINLOCK_INIT { ATOMIC_INIT(0), ATOMIC_INIT(1), 16, 0}
/// Macro for irqsave spinlock initialization
#define SPINLOCK_IRQSAVE_INIT { ATOMIC_INIT(0), ATOMIC_INIT(1), 0, 0}


#endif