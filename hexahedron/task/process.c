/**
 * @file hexahedron/task/process.c
 * @brief Main process logic
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/task/process.h>
#include <kernel/arch/arch.h>
#include <kernel/loader/elf_loader.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/fs/vfs.h>
#include <kernel/debug.h>
#include <kernel/panic.h>

#include <structs/tree.h>
#include <structs/list.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Process tree */
tree_t *process_tree = NULL;

/* PID bitmap */
uint32_t *pid_bitmap = NULL;

/* Task switches */
volatile int task_switches = 0;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "TASK:PROCESS", __VA_ARGS__)

/**
 * @brief Initialize the process system, starting the idle process
 * 
 * This will NOT switch to the next task! Instead it will prepare the system
 * by creating the necessary structures and allocating an idle task for the
 * BSP.
 */
void process_init() {
    // Initialize tree
    process_tree = tree_create("process tree");
    
    // Initialize scheduler
    scheduler_init();

    LOG(INFO, "Process system initialized\n");
}

/**
 * @brief Switch to the next thread in the queue
 * 
 * For CPU cores: This is jumped to immediately after AP initialization, specifically 
 * after the idle task has been created (through process_spawnIdleTask). It will automatically
 * enter the scheduling loop, and when a new process spawns the core can get it.
 * 
 * @warning Don't call this unless you know what you're doing. Use @c process_yield
 */
void __attribute__((noreturn)) process_switchNextThread() {
    // Get next thread in queue
    thread_t *next_thread = scheduler_get();
    if (!next_thread) {
        kernel_panic_extended(SCHEDULER_ERROR, "scheduler", "*** No thread was found in the scheduler (or something has been corrupted). Got thread %p.\n", next_thread);
    }

    // Update CPU variables
    current_cpu->current_thread = next_thread;
    current_cpu->current_process = next_thread->parent;

    // Setup page directory
    mem_switchDirectory(current_cpu->current_thread->dir);

    // On your mark...
    arch_prepare_switch(current_cpu->current_thread);

    // Get set..
    __sync_or_and_fetch(&current_cpu->current_thread->status, THREAD_STATUS_RUNNING);
    
    // Go!
    // #ifdef __ARCH_I386__
    // dprintf(DEBUG, "Thread %p (%s), dump context: IP %p SP %p BP %p\n", current_cpu->current_thread, next_thread->parent->name, current_cpu->current_thread->context.eip, current_cpu->current_thread->context.esp, current_cpu->current_thread->context.ebp);
    // #else
    // dprintf(DEBUG, "Thread %p (%s), dump context: IP %p SP %p BP %p\n", current_cpu->current_thread, current_cpu->current_thread->parent->name, current_cpu->current_thread->context.rip, current_cpu->current_thread->context.rsp, current_cpu->current_thread->context.rbp);
    // #endif

    task_switches += 1;
    arch_load_context(&current_cpu->current_thread->context);
    __builtin_unreachable();
}

/**
 * @brief Yield to the next task in the queue
 * 
 * This will yield current execution to the next available task, but will return when
 * this process is loaded by @c process_switchNextThread
 * 
 * @param reschedule Whether to readd the process back to the queue, meaning it can return whenever and isn't waiting on something
 */
void process_yield(uint8_t reschedule) {
    // Do we even have a thread?
    if (current_cpu->current_thread == NULL) {
        // Just switch to next thread
        process_switchNextThread();
    }

    // Thread no longer has any time to execute. Save FPU registers
    // TODO: DESPERATELY move this to context structure.
    asm volatile ("fxsave (%0)" :: "r"(current_cpu->current_thread->fp_regs));

    // Equivalent to a setjmp, use arch_save_context() to save our context
    if (arch_save_context(&current_cpu->current_thread->context) == 1) {
        // We are back, and were chosen to be executed. Return
        asm volatile ("fxrstor (%0)" :: "r"(current_cpu->current_thread->fp_regs));
        return;
    }
    
    // NOTE: Normally we would call process_switchNextThread but that will cause a critical error. See reschedule part of this function

    // Get current thread
    thread_t *prev = current_cpu->current_thread;

    // Get next thread in queue
    thread_t *next_thread = scheduler_get();
    if (!next_thread) {
        // next_thread = current_cpu->idle_process->main_thread;
        kernel_panic_extended(SCHEDULER_ERROR, "scheduler", "*** No thread was found in the scheduler (or something has been corrupted). Got thread %p.\n", next_thread);
    }

    // Update CPU variables
    current_cpu->current_thread = next_thread;
    current_cpu->current_process = next_thread->parent;

    // Setup page directory
    // TODO: Test this. Is it possible mem_getCurrentDirectory != mem_getKernelDirectory?
    if (next_thread->dir || mem_getCurrentDirectory() != mem_getKernelDirectory()) mem_switchDirectory(current_cpu->current_thread->dir);

    // On your mark... (load kstack)
    arch_prepare_switch(current_cpu->current_thread);

    // Get set..
    __sync_or_and_fetch(&current_cpu->current_thread->status, THREAD_STATUS_RUNNING);
    
    // Go!
    #ifdef __ARCH_I386__
    dprintf(DEBUG, "Thread %p (%s), dump context: IP %p SP %p BP %p\n", current_cpu->current_thread, next_thread->parent->name, current_cpu->current_thread->context.eip, current_cpu->current_thread->context.esp, current_cpu->current_thread->context.ebp);
    #else
    dprintf(DEBUG, "Thread %p (%s), dump context: IP %p SP %p BP %p\n", current_cpu->current_thread, current_cpu->current_thread->parent->name, current_cpu->current_thread->context.rip, current_cpu->current_thread->context.rsp, current_cpu->current_thread->context.rbp);
    #endif

    // Reschedule thread now. This should leave a VERY slim time window for another CPU to pick up the thread
    if (prev && reschedule) {
        // !!!: It is possible for a race condition to occur here. It is very unlikely, but possible.
        // !!!: If another CPU picks this thread up and somehow manages to switch to it faster than we can, it will corrupt the stack and cause hell.
        // !!!: This is why we can't call process_switchNextThread. Sorry, not sorry.
        scheduler_insertThread(prev);
    }

    task_switches += 1;
    arch_load_context(&current_cpu->current_thread->context);
    __builtin_unreachable();
}

/**
 * @brief Allocate a new PID from the PID bitmap
 */
pid_t process_allocatePID() {
    if (pid_bitmap == NULL) {
        // Create bitmap
        pid_bitmap = kmalloc(PROCESS_PID_BITMAP_SIZE);
        memset(pid_bitmap, 0, PROCESS_PID_BITMAP_SIZE);
    }

    for (uint32_t i = 0; i < PROCESS_PID_BITMAP_SIZE; i++) {
        for (uint32_t j = 0; j < sizeof(uint32_t) * 8; j++) {
            // Check each bit in the PID bitmap
            if (!(pid_bitmap[i] & (1 << j))) {
                // This is a free PID, allocate and return it
                pid_bitmap[i] |= (1 << j);
                return (i * (sizeof(uint32_t) * 8)) + j;
            }
        }
    }

    // Out of PIDs
    kernel_panic_extended(SCHEDULER_ERROR, "process", "*** Out of process PIDs.\n");
    return 0;
}

/**
 * @brief Free a PID after process destruction
 * @param pid The PID to free
 */
void process_freePID(pid_t pid) {
    if (!pid_bitmap) return; // ???

    // To get the index in the bitmap, round down PID
    int bitmap_idx = (pid / 32) * 32;
    pid_bitmap[bitmap_idx] &= ~(1 << (pid - bitmap_idx));
}

/**
 * @brief Internal method to create a new process
 * @param name The name of the process (will be strdup'd)
 * @param flags The flags to use for the process
 * @param priority The priority of the process
 */
static process_t *process_createStructure(char *name, unsigned int flags, unsigned int priority) {
    process_t *process = kmalloc(sizeof(process_t));
    memset(process, 0, sizeof(process_t));

    // Setup some variables
    process->name = strdup(name);
    process->flags = flags;
    process->priority = priority;
    process->gid = process->uid = 0;
    process->pid = process_allocatePID();

    // Create process' kernel stack
    process->kstack = mem_allocate(0, PROCESS_KSTACK_SIZE, MEM_ALLOC_HEAP, MEM_PAGE_KERNEL) + PROCESS_KSTACK_SIZE;
    dprintf(DEBUG, "Process '%s' has had its kstack %p allocated in page directory %p\n", name, process->kstack, current_cpu->current_dir);
    
    return process;
}

/**
 * @brief Idle process function
 */
static void kernel_idle() {
    // Pause execution
    arch_pause();

    // For the kidle process, this can serve as total "cycles"
    current_cpu->current_thread->total_ticks++; 

    // Switch to next thread
    process_switchNextThread();
}

/**
 * @brief Create a new idle process
 * 
 * Creates and returns an idle process. All this process does is repeatedly
 * call @c arch_pause and try to switch to the next thread.
 * 
 * @note    This process should not be added to the process tree. Instead it is
 *          kept in the main process data structure, and will be switched to
 *          when there are no other processes to go to.
 */
process_t *process_spawnIdleTask() {
    // Create new process
    process_t *idle = process_createStructure("idle", PROCESS_KERNEL | PROCESS_STARTED | PROCESS_RUNNING, PRIORITY_LOW);
    
    // !!!: Hack
    process_freePID(idle->pid);
    idle->pid = -1; // Not actually a process

    // Create a new thread
    idle->main_thread = thread_create(idle, NULL, (uintptr_t)&kernel_idle, THREAD_FLAG_KERNEL);

    return idle;
}

/**
 * @brief Spawn a new init process
 * 
 * Creates and returns an init process. This process has no context, and is basically
 * an empty shell. Rather, when @c process_execute is called, it replaces the current process' (init)
 * threads and sections with the process to execute for init.
 */
process_t *process_spawnInit() {
    // Create a new process
    process_t *init = process_createStructure("init", PROCESS_STARTED | PROCESS_RUNNING, PRIORITY_HIGH);

    // Set as parent node (all processes stem from this one)
    tree_set_parent(process_tree, (void*)init);
    init->node = process_tree->root;

    // Done
    return init;
}

/**
 * @brief Create a new process
 * @param name The name of the process
 * @param flags The flags of the process
 * @param priority The priority of the process 
 */
process_t *process_create(char *name, int flags, int priority) {
    return process_createStructure(name, flags, priority);
}

/**
 * @brief Execute a new ELF binary for the current process (execve)
 * @param file The file to execute
 * @param argc The argument count
 * @param argv The argument list
 * @returns Error code
 */
int process_execute(fs_node_t *file, int argc, char **argv) {
    if (!file) return -EINVAL;
    if (!current_cpu->current_process) return -EINVAL; // TODO: Handle this better

    // Check the ELF binary
    if (elf_check(file, ELF_EXEC) == 0) {
        // Not a valid ELF binary
        LOG(ERR, "Invalid ELF binary detected when trying to start execution\n");
        return -EINVAL;
    }

    // Destroy previous threads
    // TODO

    // Create a new main thread with a blank entrypoint
    mem_switchDirectory(NULL);
    current_cpu->current_process->main_thread = thread_create(current_cpu->current_process, mem_clone(NULL), 0x0, THREAD_FLAG_DEFAULT);

    // Switch to directory
    mem_switchDirectory(current_cpu->current_process->main_thread->dir);

    // Load file into memory
    // TODO: This runs check twice (redundant)
    uintptr_t elf_binary = elf_load(file, ELF_USER);

    // Success?
    if (elf_binary == 0x0) {
        // Something happened...
        LOG(ERR, "ELF binary failed to load properly (but is valid?)\n");
        return -EINVAL;
    }

    // Get the entrypoint
    uintptr_t process_entrypoint = elf_getEntrypoint(elf_binary);
    arch_initialize_context(current_cpu->current_process->main_thread, process_entrypoint, current_cpu->current_process->main_thread->stack);

    // Ready, we own this process.
    current_cpu->current_thread = current_cpu->current_process->main_thread;

    // Enter
    LOG(DEBUG, "Launching new ELF process\n");
    arch_prepare_switch(current_cpu->current_thread);
    arch_start_execution(process_entrypoint, current_cpu->current_thread->stack);

    return 0;
} 

/**
 * @brief Exiting from a process
 * @param process The process to exit from, or NULL for current process
 * @param status_code The status code
 */
void process_exit(process_t *process, int status_code) {
    if (!process) process = current_cpu->current_process;
    if (!process) kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, "process", "*** Cannot exit from non-existant process\n");
    
    int is_current_process = (process == current_cpu->current_process);

    // The scheduler itself ignores the process state, so mark it as stopped
    // __sync_or_and_fetch(&process->flags, PROCESS_STOPPED);
    process->flags |= PROCESS_STOPPED;

    // Now we need to mark all threads of this process as stopping. This will ensure that memory is fully separate
    if (process->main_thread) __sync_or_and_fetch(&process->main_thread->status, THREAD_STATUS_STOPPING);
    
    if (process->thread_list && process->thread_list->length) {
        foreach(thread_node, process->thread_list) {
            if (thread_node->value) {
                thread_t *thr = (thread_t*)thread_node->value;
                __sync_or_and_fetch(&thr->status, THREAD_STATUS_STOPPING);
            }
        }
    }


    // Destroy thread list
    if (process->thread_list) list_destroy(process->thread_list, false);

    LOG(INFO, "Freeing memory for process \"%s\"...\n", process->name);

    // Free whatever memory remains
    if (process->node) {
        tree_remove(process_tree, process->node);
        kfree(process->node);
    }

    kfree(process->name);
    mem_free(process->kstack - PROCESS_KSTACK_SIZE, PROCESS_KSTACK_SIZE, MEM_DEFAULT); // !!!: This needs to be replaced
    kfree(process);

    // To the next process we go
    if (is_current_process) process_yield(1);   // Yield will reschedule us, scheduler will catch and destroy us.
    process_switchNextThread();
}