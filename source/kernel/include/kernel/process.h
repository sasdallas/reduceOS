// process.h - header file for the task scheduler/switcher

#ifndef PROCESS_H
#define PROCESS_H

// Includes
#include <stdint.h>
#include <time.h>
#include <kernel/vmm.h>

// Definitions
#define MAX_THREADS 5               // Maximum threads a process can have



// Enums
typedef enum {
    SLEEP = 0,
    ACTIVE = 1
} process_state;


// Typedefs

typedef struct thread_t; // Prototype

typedef struct {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t edi;
    uint32_t esi;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t flags;
    // TODO: Add more
} trapFrame_t;

// This is the main process structure - all processes require this.
typedef struct process {
    int pid;                        // Process ID
    int priority;                   // Process priority
    pagedirectory_t pageDirectory;  // Process pgage directory
    process_state state;            // The current state of the process
    thread_t threads[MAX_THREADS];  // Process thread list
    time_t process_time;            // Time the process has used
} process_t;

typedef struct {
    process_t *parent;              // Parent process of a thread
    void *initial_stack;            // Initial process stack
    void *stack_limit;              // Stack limit
    void *kernel_stack;             // Thread's kernel stack
    uint32_t priority;              // Priority of the thread
    int state;                      // Thread state (todo - make an enum for this)
    trapFrame_t frame;              // Trap frame
} thread_t;


#endif