/**
 * @file libpolyhedron/string/strncmp.c
 * @brief String compare function
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

int strncmp(const char* str1, const char* str2, size_t size) {
    if (!size) return 0;

    while (*str1 == *str2) {
        if (!size || !*str1) break; // Terminate if nothing left

        str1++;
        str2++;
        size--;
    }

    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

int strcmp(const char* str1, const char* str2) {
    while (*str1 == *str2) {
        str1++;
        str2++;
    }

    return *(unsigned char*)str1 - *(unsigned char*)str2;
}