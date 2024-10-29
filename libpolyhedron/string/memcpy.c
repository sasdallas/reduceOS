/**
 * @file libpolyhedron/string/memcpy.c
 * @brief memcpy() function
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <string.h>


void* memcpy(void* __restrict destination_ptr, const void* __restrict source_ptr, size_t size) {
    unsigned char* destination = (unsigned char*)destination_ptr;
    const unsigned char* source = (const unsigned char*)source_ptr;

    for (size_t i = 0; i < size; i++)
        destination[i] = source[i];

    return destination_ptr;
}