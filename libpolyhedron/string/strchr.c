/**
 * @file libpolyhedron/string/strchr.c
 * @brief strchr
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

char *strchr(const char *str, int character) {
    char *ptr = (char*)str;

    while (*ptr) {
        if (*ptr == character) return ptr;
        ptr++;
    }

    return NULL;
}