// spinlock.h - spinlock manager/functions
// This implementation of spinlock is based of eduOS by RWTH-OS.

#ifndef SPINLOCK_H
#define SPINLOCK_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/atomic.h" // Atomic functions
#include "include/libc/spinlock_types.h" // Definition of the actual spinlock_t structure.

// Inline functions

extern task_t *currentTask;

inline static int spinlock_init(spinlock_t* s) {
	if (!s)
		return -22;

	atomic_int32_set(&s->queue, 0);
	atomic_int32_set(&s->dequeue, 1);
	s->owner = 16;
	s->counter = 0;

	return 0;
}

inline static int spinlock_destroy(spinlock_t* s) {
	if (!s)
		return -22;

	s->owner = 16;
	s->counter = 0;

	return 0;
}

inline static int spinlock_lock(spinlock_t* s) {
	int32_t ticket;

	if (!s)
		return -22;

	if (s->owner == currentTask->id) {
		s->counter++;
		return 0;
	}

	ticket = atomic_int32_add(&s->queue, 1);
	while(atomic_int32_read(&s->dequeue) != ticket) {
		asm volatile ("pause");
	}
	s->owner = currentTask->id;
	s->counter = 1;

	return 0;
}


inline static int spinlock_unlock(spinlock_t* s) {
	if (!s)
		return -22;

	s->counter--;
	if (!s->counter) {
		s->owner = 16;
		atomic_int32_inc(&s->dequeue);
	}

	return 0;
}


inline static int spinlock_irqsave_init(spinlock_irqsave_t* s) {
	if (!s)
		return -22;

	atomic_int32_set(&s->queue, 0);
	atomic_int32_set(&s->dequeue, 1);
	s->flags = 0;
	s->counter = 0;

	return 0;
}

inline static int spinlock_irqsave_destroy(spinlock_irqsave_t* s) {
	if (!s)
		return -22;

	s->flags = 0;
	s->counter = 0;

	return 0;
}

inline static int spinlock_irqsave_lock(spinlock_irqsave_t* s) {
	int32_t ticket;
	uint8_t flags;

	if (!s)
		return -22;


	// Disable nested IRQs
    size_t tmp;
    asm volatile ("pushf; cli; pop %0" : "=r"(tmp) :: "memory");
    if (tmp & (1 << 9)) flags = 1;
    else flags = 0;


	if (s->counter == 1) {
		s->counter++;
		return 0;
	}

	ticket = atomic_int32_add(&s->queue, 1);
	while (atomic_int32_read(&s->dequeue) != ticket) {
		asm volatile ("pause");
	}

	s->flags = flags;
	s->counter = 1;

	return 0;
}


inline static int spinlock_irqsave_unlock(spinlock_irqsave_t* s) {
	uint8_t flags;

	if (!s)
		return -22;

	s->counter--;
	if (!s->counter) {
		flags = s->flags;
		s->flags = 0;
                atomic_int32_inc(&s->dequeue);
		asm volatile ("sti" ::: "memory");
	}

	return 0;
}


#endif