/**
 * @file libpolyhedron/string/strcspn.c
 * @brief strcspn and strspn
 * 
 * 
 * @copyright
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2011-2021 K. Lange
 */


#include <string.h>
 
#define BITOP(A, B, OP) \
    ((A)[(size_t)(B) / (8 * sizeof *(A))] OP (size_t)1<<((size_t)(B)%(8*sizeof *(A))))

size_t strspn(const char *str, const char *c) {
    // First check if c is even available
    if (!c[0]) return 0;

    const char *a = str;

    // Now check if there is a second character in c.
    if (!c[1]) {
        // No, we're only looking for one character.
        for (; *str && *str != *c; str++);
        return str-a;
    }

    // Create a bitmask
    size_t byteset[32 / sizeof(size_t)] = { 0 };
    for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++);

    // AND str with the bitmask
    for (; *str && BITOP(byteset, *(unsigned char *)str, &); str++);

    return str-a;
} 

size_t strcspn(const char *str, char *c) {
    const char *a = str;

    if (c[0] && c[1]) {
        // The character string is more than 1 character, so we actually have to do work.
        // We can build a bitmask and use that.
        size_t byteset[32 / sizeof(size_t)] = { 0 };
        for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++);
    
        // Now use the bitmask with an AND operator
        for (; *str && !BITOP(byteset, *(unsigned char *)str, &); str++);

        // Return a pointer.
        return str-a;
    }

    // c is only one character, use strchrnul
    return strchrnul(str, *c) - a;
}