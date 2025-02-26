/**
 * @file hexahedron/task/thread.c
 * @brief Main thread logic
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
#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/drivers/clock.h>
#include <string.h>

/**
 * @brief Create a new thread (internal)
 * @param parent The parent process of the thread
 * @param dir The directory of the thread
 * @param status The current status of the thread
 * @param flags The flags of the thread
 * 
 * @note No ticks are set and context will need to be saved
 */
static thread_t *thread_createStructure(process_t *parent, page_t *dir, int status,  int flags) {
    thread_t *thr = kmalloc(sizeof(thread_t));
    memset(thr, 0, sizeof(thread_t));
    thr->parent = parent;
    thr->status = status;
    thr->dir = dir;
    thr->flags = flags;

    // Thread ticks aren't updated because they should ONLY be updated when scheduler_insertThread is called
    return thr;
}

/**
 * @brief Create a new thread
 * @param parent The parent process of the thread
 * @param dir Directory to use (for new threads being used as main, mem_clone() this first, else refcount the main thread's directory)
 * @param entrypoint The entrypoint of the thread (you can also set this later)
 * @param flags Flags of the thread
 * @returns New thread pointer, just save context & add to scheduler queue
 */
thread_t *thread_create(struct process *parent, page_t *dir, uintptr_t entrypoint, int flags) {
    // Create thread
    thread_t *thr = thread_createStructure(parent, dir, THREAD_STATUS_RUNNING, flags);
    
    // Switch directory to directory (as we will be mapping in it)
    mem_switchDirectory(dir);

    if (!(flags & THREAD_FLAG_KERNEL)) {
        // Allocate usermode stack 
        // !!!: VERY BAD, ONLY USED FOR TEMPORARY TESTING
        thr->stack = mem_allocate(0, THREAD_STACK_SIZE*2, MEM_ALLOC_HEAP, MEM_DEFAULT) + THREAD_STACK_SIZE;
        mem_allocatePage(mem_getPage(dir, thr->stack - THREAD_STACK_SIZE, MEM_CREATE), MEM_PAGE_NOALLOC); // Reallocates pages as usermode
        mem_allocatePage(mem_getPage(dir, thr->stack, MEM_CREATE), MEM_PAGE_NOALLOC); // Reallocates pages as usermode
    } else {
        // Don't bother, use the parent's kernel stack
        thr->stack = parent->kstack; 
    }

    // Initialize thread context
    arch_initialize_context(thr, entrypoint, thr->stack);

    // Switch back out of the directory back to kernel
    mem_switchDirectory(NULL);

    return thr;
}

