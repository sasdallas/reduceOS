/**
 * @file hexahedron/misc/semaphore.c
 * @brief Semaphore synchronization system 
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/misc/semaphore.h>
#include <kernel/panic.h>
#include <kernel/mem/alloc.h>

/**
 * @brief Initialize and create a semaphore
 * @param name          Optional semaphore name (for debugging)
 * @param value         The initialization value of the semaphore
 * @param max_value     The maximum value of the semaphore (afterwards, signal will wait)
 */
semaphore_t *semaphore_init(char *name, int value, int max_value) {
    semaphore_t *output = kmalloc(sizeof(semaphore_t));
    output->lock = spinlock_create("sem_lock");
    output->value = value;
    output->max_value = max_value;
    output->name = name;
    return output;
}

/**
 * @brief Wait on the semaphore
 * @param semaphore     The semaphore to use
 * @param items         The amount of items to take from the semaphore
 * @returns 0 on success
 */
int semaphore_wait(semaphore_t *semaphore, int items) {
    // Lock the semaphore
    spinlock_acquire(semaphore->lock);

    // Wait won't fault on not enough items taken
    if (semaphore->value > 0) {
        if (semaphore->value > items) semaphore->value -= items;
        else semaphore->value = 0;
    } else {
        // Wait how do we do that again
        // Yeah I don't know what I am doing
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "semaphore", "*** Semaphore underflowed max value. No thread implementation is available\n"); 
    }

    // Release the semaphore
    spinlock_release(semaphore->lock);

    return 0;
}

/**
 * @brief Signal to the semaphore
 * @param semaphore     The semaphore to use
 * @param items         The amount of items to add to the semaphore
 * @returns 0 on success
 */
int semaphore_signal(semaphore_t *semaphore, int items) {
    // Lock the semaphore
    spinlock_acquire(semaphore->lock);

    // Okay, do we need to wait?
    if (semaphore->max_value && semaphore->value >= semaphore->max_value) {
        // Wait how do we do that again
        // Yeah I don't know what I am doing
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "semaphore", "*** Semaphore exceeded max_value. No thread implementation is available.\n"); 
    } else {
        // Just add to semaphore. Make sure to limit though to prevent chaos
        if (semaphore->value + items > semaphore->max_value) {
            semaphore->value = semaphore->max_value;
        } else {
            semaphore->value += items;
        }
    }

    // Release the semaphore
    spinlock_release(semaphore->lock);

    return 0;
}

/**
 * @brief Get the semaphore's items
 * @param semaphore     The semaphore to use
 * @returns The amount of items in the semaphore
 */
int semaphore_getItems(semaphore_t *semaphore) {
    return semaphore->value;
}

/**
 * @brief Destroy the semaphore
 * @param semaphore     The semaphore to use
 */
void semaphore_destroy(semaphore_t *semaphore) {
    // !!!: Free semaphore->name?
    kfree(semaphore->lock);
    kfree(semaphore);
}