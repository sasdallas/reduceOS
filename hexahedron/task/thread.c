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
 * 
 * @note No ticks are set and context will need to be saved
 */
static thread_t *thread_createStructure(process_t *parent, page_t *dir, int status) {
    thread_t *thr = kmalloc(sizeof(thread_t));
    memset(thr, 0, sizeof(thread_t));
    thr->parent = parent;
    thr->status = status;
    thr->dir = dir;

    // Thread ticks aren't updated because they should ONLY be updated when scheduler_insertThread is called
    return thr;
}

/**
 * @brief Create a new thread
 * @param proc The process to append the thread to (if it doesn't have a main thread, this thread will automatically be set as main thread)
 * @param dir Directory TO CLONE. The thread will hold a CLONED copy of this directory.
 * @returns New thread pointer, save context & add to scheduler queue
 */
thread_t *thread_create(process_t *parent, page_t *dir) {
    page_t *dir_actual = mem_clone(dir);
    thread_t *thr = thread_createStructure(parent, dir_actual, THREAD_STATUS_RUNNING);

    // Set thread in parent
    if (parent->main_thread) {
        if (!parent->thread_list) parent->thread_list = list_create("thread list");
        list_append(parent->thread_list, (void*)thr);
    } else {
        parent->main_thread = thr;
    }

    return thr;
}


