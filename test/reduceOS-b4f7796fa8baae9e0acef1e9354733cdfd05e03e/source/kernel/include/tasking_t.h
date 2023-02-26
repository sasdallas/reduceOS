// tasking_t.h - contains definitions for task_t, task constants, and multiple other things all relating either to the TSS or a task.

#ifndef TASKING_T_H
#define TASKING_T_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/stddef.h" // Misc. definitions
#include "include/libc/spinlock_types.h"

// Definitions

// Task states
#define TASK_INVALID 0
#define TASK_READY 1
#define TASK_RUNNING 2
#define TASK_BLOCKED 3
#define TASK_FINISHED 4
#define TASK_IDLE 5

// Other things about tasks
#define TASK_DEFAULT_FLAGS 0
#define TASK_FPU_INIT (1 << 0)
#define TASK_FPU_USED (1 << 1)

// Priorities (max, realtime, high, normal, low, idle)
#define MAX_PRIORITY 31
#define REALTIME_PRIORITY 31
#define HIGH_PRIORITY 16
#define NORMAL_PRIORITY 8
#define LOW_PRIORITY 1
#define IDLE_PRIORITY 0

// Typedefs

typedef int (*entryPoint_t)(void*); // Entry point declaration.
typedef unsigned int taskID_t; // Task identifier declaration.


// Task declaration.
typedef struct task {
    taskID_t id __attribute__ ((aligned (64))); // Task ID (or position in the task table)
    uint32_t taskStatus; // Task status. Can be any of the above states.
    size_t *lastStackPointer; // Copy of the stack pointer before a context switch.
    void *stackStart; // Start address of the stack.
    uint8_t statusFlags; // Additional status flags.
    uint8_t taskPriority; // Task priority.
    size_t pageMap; // Physical address of the root page table.
    struct task *next; // The next task in the queue.
    struct task *prev; // Previous task in the queue.
} task_t;

typedef struct {
    task_t *first;
    task_t *last;
} taskList_t;

typedef struct {
    task_t *idle __attribute__ ((aligned (64)));
    task_t *oldTask;
    uint32_t numTasks;
    uint32_t priorityBitmap;
    taskList_t queue[31];
    spinlock_irqsave_t lock;
} readyQueues_t;

typedef unsigned int tid_t;

#endif