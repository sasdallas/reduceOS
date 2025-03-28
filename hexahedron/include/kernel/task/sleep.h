/**
 * @file hexahedron/include/kernel/task/block.h
 * @brief Thread blocking/sleeping 
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_TASK_BLOCK_H
#define KERNEL_TASK_BLOCK_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/arch/arch.h>
#include <kernel/mem/mem.h>
#include <structs/node.h>

/**** DEFINITIONS ****/

#define SLEEP_FLAG_NOCOND           0       // There is no condition under which the thread should wakeup. Dead thread. Should only be used for debugging.
#define SLEEP_FLAG_WAKEUP           1       // Whatever the case, wake it up NOW!


/**** TYPES ****/

/**
 * @brief Sleep condition function
 * @param context Context provided by @c thread_blockXXX
 * @returns 0 on not ready to resume, 1 on ready to resume
 */
typedef int (*sleep_condition_t)(void *context);

/**
 * @brief Sleeper structure
 */
typedef struct thread_sleep {
    struct thread *thread;                  // Thread which is sleeping
    node_t *node;                           // Assigned node in the sleeping queue
    volatile int sleep_state;               // Sleeping flags

    // Conditional-sleeping threads
    sleep_condition_t condition;            // Condition on which to wakeup
    void *context;                          // Context for said condition

    // Specific to threads sleeping for time
    unsigned long seconds;                  // Seconds on which to wakeup
    unsigned long subseconds;               // Subseconds on which to wakeup
} thread_sleep_t;


/**** FUNCTIONS ****/


#endif