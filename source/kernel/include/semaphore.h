// semaphore.h - header file containing inline functions to manage the semaphore
// This file is written by eduOS by RWTH-OS, I did not write it.

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

// Includes
#include "include/semaphore_t.h" // Semaphore typedefs
#include "include/libc/stdint.h" // Integer definitions
#include "include/tasking.h" // Rescheduler


// Note: The number 16 is a reference to maximum tasks.

/** @brief Semaphore initialization
 *
 * Always init semaphores before use!
 *
 * @param s Pointer to semaphore structure to initialize
 * @param v Resource count
 *
 * @return
 * - 0 on success
 * - -22 on invalid argument
 */
inline static int sem_init(sem_t* s, unsigned int v) {
	unsigned int i;

	if (!s)
		return -22;

	s->value = v;
	s->pos = 0;
	for(i=0; i<16; i++)
		s->queue[i] = 16;
	spinlock_irqsave_init(&s->lock);

	return 0;
}

/** @brief Destroy semaphore
 * @return
 * - 0 on success
 * - -22 on invalid argument
 */
inline static int sem_destroy(sem_t* s) {
	if (!s)
		return -22;

	spinlock_irqsave_destroy(&s->lock);

	return 0;
}

/** @brief Nonblocking trywait for sempahore
 *
 * Will return immediately if not available
 *
 * @return
 * - 0 on success (You got the semaphore)
 * - -22 on invalid argument
 * - -ECANCELED on failure (You still have to wait)
 */
inline static int sem_trywait(sem_t* s) {
	int ret = -140;

	if (!s)
		return -22;

	spinlock_irqsave_lock(&s->lock);
	if (s->value > 0) {
		s->value--;
		ret = 0;
	}
	spinlock_irqsave_unlock(&s->lock);

	return ret;
}

/** @brief Blocking wait for semaphore
 *
 * @param s Address of the according sem_t structure
 * @return 
 * - 0 on success
 * - -22 on invalid argument
 * - -ETIME on timer expired
 */
inline static int sem_wait(sem_t* s) {
	if (!s)
		return -22;

next_try1:
	spinlock_irqsave_lock(&s->lock);
	if (s->value > 0) {
		s->value--;
		spinlock_irqsave_unlock(&s->lock);
	} else {
		s->queue[s->pos] = currentTask->id;
		s->pos = (s->pos + 1) % 16;
		task_blockTask();
		spinlock_irqsave_unlock(&s->lock);
		task_reschedule();
		goto next_try1;
	}

	return 0;
}

/** @brief Give back resource 
 * @return
 * - 0 on success
 * - -22 on invalid argument
 */
inline static int sem_post(sem_t* s) {
	if (!s)
		return -22;

	spinlock_irqsave_lock(&s->lock);
	if (s->value > 0) {
		s->value++;
		spinlock_irqsave_unlock(&s->lock);
	} else {
		unsigned int k, i;

		s->value++;
		i = s->pos;
		for(k=0; k<16; k++) {
			if (s->queue[i] < 16) {
				task_wakeupTask(s->queue[i]);
				s->queue[i] = 16;
				break;
			}
			i = (i + 1) % 16;
		}
		spinlock_irqsave_unlock(&s->lock);
	}

	return 0;
}

#endif