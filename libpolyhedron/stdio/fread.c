/**
 * @file libpolyhedron/stdio/fread.c
 * @brief fread
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <stdio.h>

extern size_t __fileio_read_bytes(FILE *f, char *buf, size_t size);

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!size || !nmemb || !stream) return 0;


    // fread reads nmemb objects of size size to f
    char *p = (char*)ptr;
    for (size_t i = 0; i < nmemb; i++) {
        size_t r = __fileio_read_bytes(stream, p, size);
        if (r < size) return i; // Out of objects
        p += size;
    }

    return nmemb; 
}