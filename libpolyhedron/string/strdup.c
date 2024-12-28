/**
 * @file libpolyhedron/string/strdup.c
 * @brief strdup
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
#include <stdlib.h>

char *strdup(const char *str) {
    char *new_str = malloc(strlen(str));
    if (!new_str) return NULL;
    strcpy(new_str, str);
    return new_str;
}