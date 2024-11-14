/**
 * @file hexahedron/mem/alloc.c
 * @brief Allocator management system for Hexahedron.
 * 
 * Multiple allocators are suported for Hexahedron (not simultaneous, at compile-time)
 * This allocator system handles debug, feature support, forwarding, profiling, etc.
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/panic.h>
#include <kernel/debug.h>


/* Internal copy of the allocator's data */
static allocator_info_t *alloc_info = NULL; // !!!: What if a bad allocator changes this after giving it to us? 

/* Current profiling data */
static profile_info_t *profile_data = NULL;


/** FORWARDER FUNCTIONS **/

/**
 * @brief Allocate kernel memory
 * @param size The size of the allocation
 * 
 * @returns A pointer. It will crash otherwise.
 */
__attribute__((malloc)) void *kmalloc(size_t size) {
    void *ptr = alloc_malloc(size);
    return ptr;
}

/**
 * @brief Reallocate kernel memory
 * @param ptr A pointer to the previous structure
 * @param size The new size of the structure.
 * 
 * @returns A pointer. It will crash otherwise.
 */
__attribute__((malloc)) void *krealloc(void *ptr, size_t size) {
    void *ret_ptr = alloc_realloc(ptr, size);
    return ret_ptr;
}

/**
 * @brief Contiguous allocation function
 * @param elements The amount of elements to allocate
 * @param size The size of each element
 * 
 * @returns A pointer. It will crash otherwise.
 */
__attribute__((malloc)) void *kcalloc(size_t elements, size_t size) {
    void *ptr = alloc_calloc(elements, size);
    return ptr;
}

/**
 * @brief Page-aligned memory allocator
 * @param size The size to allocate.
 * 
 * @note Do not rely on this! Allocators can choose not to provide this!
 * @returns A pointer or crashes with an unimplemented exception.
 */
__attribute__((malloc)) void *kvalloc(size_t size) {
    if (alloc_canHasValloc()) {
        void *ptr = alloc_valloc(size);
        return ptr;
    } else {
        kernel_panic_extended(UNSUPPORTED_FUNCTION_ERROR, "alloc", "valloc() is not supported in this context.\n");
        __builtin_unreachable();
    }
}

/**
 * @brief Free kernel memory
 * @param ptr A pointer to the previous memory
 */
void kfree(void *ptr) {
    alloc_free(ptr);
}

/** ALLOCATOR-MANAGEMENT FUNCTIONS **/

/**
 * @brief valloc?
 */
int alloc_canHasValloc() {
    if (alloc_info == NULL) {
        /* Get some */
        alloc_info = alloc_getInfo();
    }

    return alloc_info->support_valloc;
}


/**
 * @brief Start profiling the memory system.
 * 
 * This will initialize the memory system in such a way that every call to malloc/realloc/calloc/whatever
 * will be logged, and their results will be analyzed.
 * 
 * This is a performance checking function, used to compare different allocators or
 * alternatively to find memory leaks.
 * 
 * @note To retrieve data, call alloc_stopProfiling.
 * 
 * @param force_begin_profiling If another CPU has already started the profiling process,
 *                              this will try to acquire the spinlock and halt the CPU until the
 *                              current process is finished.
 * 
 * @warning You can hang the system if not careful with this.
 *          Use this sparingly.
 * 
 * @returns 0 on successful profiling start
 *          -EINPROGRESS if it was already started and @c force_begin_profiling was not specified.
 */
int alloc_startProfiling(int force_begin_profiling) {
    return -ENOTSUP; /* TODO */

    profile_data = kmalloc(sizeof(profile_info_t));
}

/**
 * @brief Stop profiling the memory system.
 * 
 * @see alloc_startProfiling for an explanation on the profiling system.
 * 
 * @returns Either a pointer to the @c profile_info_t structure or NULL.
 */
profile_info_t *alloc_stopProfiling() {
    return NULL; /* TODO */
}