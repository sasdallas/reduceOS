/**
 * @file libpolyhedron/stdlib/atoi.c
 * @brief atoi
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int atoi(const char *str) {
    if (!str) return 0;

    // First, discard as much whitespace as needed.
    char *ptr = (char*)str;
    while (isspace(*ptr)) ptr++;

    // Read the sign if needed
    int sign = 1; // default +
    if (*ptr == '-') {
        sign = -1;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }

    // Now convert!
    int value = 0;
    int base = 1;
    while (isdigit(*ptr)) {
        value += (base * (*ptr - '0'));
        base *= 10;
        ptr++;
    }

    return value * sign;
}