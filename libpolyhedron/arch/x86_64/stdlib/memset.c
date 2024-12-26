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

void *memset(void *dest, int c, size_t n) {
    asm volatile("cld; rep stosb"
	             : "=c"((int){0})
	             : "rdi"(dest), "a"(c), "c"(n)
	             : "flags", "memory", "rdi");
	return dest;
}