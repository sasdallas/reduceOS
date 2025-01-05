/**
 * @file libpolyhedron/arch/x86_64/stdlib/memset.c
 * @brief memset
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

void *memset(void* destination_ptr, int value, size_t size) {
    // It's faster to embed assembly like this and use "rep stosb".
	// This code needs to be improved and cleaned up a bit though. Intellisense does not like the ';'
    asm volatile("cld; rep stosb"
	             : "=c"((int){0})
	             : "rdi"(destination_ptr), "a"(value), "c"(size)
	             : "flags", "memory", "rdi");
	return destination_ptr;
}