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
#include <kernel/fs/vfs.h>
#include <structs/tree.h>

/**** DEFINITIONS ****/

#define PROCESS_MAX_PIDS            32768                                       // Maximum amount of PIDs. The kernel uses a bitmap to keep track of these
#define PROCESS_PID_BITMAP_SIZE     PROCESS_MAX_PIDS / (sizeof(uint32_t) * 8)   // Bitmap size

#define PROCESS_KSTACK_SIZE         8192    // Kernel stack size

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
    
    // QUEUE INFORMATION
    tree_node_t *node;      // Node in the process tree

    // THREADS
    thread_t *main_thread;  // Main thread in the process  - whatever the ELF entrypoint was
    list_t  *thread_list;   // List of threads for the process

    // OTHER
    uintptr_t kstack;       // Kernel stack (see PROCESS_KSTACK_SIZE)
} process_t;

/**** FUNCTIONS ****/

/**
 * @brief Initialize the process system, starting the idle process
 * 
 * This will NOT switch to the next task! Instead it will prepare the system
 * by creating the necessary structures and allocating an idle task for the
 * BSP.
 */
void process_init();

/**
 * @brief Switch to the next thread in the queue
 * 
 * For CPU cores: This is jumped to immediately after AP initialization, specifically 
 * after the idle task has been created (through process_spawnIdleTask). It will automatically
 * enter the scheduling loop, and when a new process spawns the core can get it.
 */
void __attribute__((noreturn)) process_switchNextThread();

/**
 * @brief Yield to the next task in the queue
 * 
 * This will yield current execution to the next available task, but will return when
 * this process is loaded by @c process_switchNextThread
 * 
 * @note    This will return eventually. It's easiest to visualize with the x86 PIT timer - when the PIT interrupts
 *          during a usermode task (CS != 0x08), it has already rescheduled (through clock_update) and will now yield the task. When
 *          it becomes time to run this thread, load_context should return here, which just returns again (and the IRQ handler jumps back to usermode process)
 */
void process_yield();

/**
 * @brief Create a new idle process
 * 
 * Creates and returns an idle process. All this process does is repeatedly
 * call @c arch_pause and try to switch to the next thread.
 * 
 * @note    This process should not be added to the process tree. Instead it is
 *          kept in the main process data structure, and will be switched to when there are no other processes
 *          to go to.
 */
process_t *process_spawnIdleTask();

/**
 * @brief Spawn a new init process
 * 
 * Creates and returns an init process. This process has no context, and is basically
 * an empty shell. Rather, when @c process_execute is called, it replaces the current process' (init)
 * threads and sections with the process to execute for init.
 */
process_t *process_spawnInit();

/**
 * @brief Execute a new ELF binary for the current process (execve)
 * @param file The file to execute
 * @param argc The argument count
 * @param argv The argument list
 * @returns Error code
 */
int process_execute(fs_node_t *file, int argc, char **argv);

#endif