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
int createKernelTask(tid_t id, entryPoint_t ep, void *args, uint8_t priority); // Create a kernel task.
int task_blockTask(); // Block the current task.
int task_wakeupTask(tid_t id); // Wakeup a blocked task
int task_createDefaultFrame(task_t *task, entryPoint_t ep, void *arg); // Creates a default frame for a new task.
void task_leaveKernelTask(); // Leave the kernel task (all tasks call this on completion).
void task_abort(); // Abort the current task.
void initMultitasking(); // Initializes multitasking by setting up the task table and registering an idle task.
void task_finishTaskSwitch(); // Finish a task switch
size_t **task_scheduler(); // The task scheduler.
void task_reschedule(); // Reschedule a task.
task_t *task_getCurrentTask(); // Get the current task.
uint32_t task_getHighestPriority(); // Returns the highest priority
size_t *task_getCurrentStack(void); // Get the current stack of whatever task is currently running.


#endif