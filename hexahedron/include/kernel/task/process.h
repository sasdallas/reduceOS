/**
 * @file hexahedron/include/kernel/task/process.h
 * @brief Main process structure
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_TASK_PROCESS_H
#define KERNEL_TASK_PROCESS_H

/**** INCLUDES ****/
#include <stdint.h>
#include <sys/types.h>
#include <structs/list.h>
#include <time.h>

#include <kernel/task/thread.h>
#include <kernel/task/scheduler.h>
#include <kernel/processor_data.h>

/**** DEFINITIONS ****/

/**** TYPES ****/

/**
 * @brief The main process type
 */
typedef struct process {
    // GENERAL INFORMATION
    pid_t pid;              // Process ID
    char *name;             // Name of the process
    uid_t uid;              // User ID of the process
    gid_t gid;              // Group ID of the process

    // SCHEDULER INFORMATION
    unsigned int flags;     // Scheduler flags (running/stopped/started) - these can also be used by other parts of code
    unsigned int priority;  // Scheduler priority, see scheduler.h
    
    // THREADS
    thread_t *main_thread;  // Main thread in the process  - whatever the ELF entrypoint was
    list_t  *thread_list;   // List of threads for the process

} process_t;

#endif