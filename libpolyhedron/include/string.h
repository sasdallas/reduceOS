/**
 * @file libpolyhedron/include/string.h
 * @brief String functions (e.g. strlen, etc.)
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <sys/cheader.h>
_Begin_C_Header

#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

int memcmp(const void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memset(void*, int, size_t);

size_t strlen(const char*);
int strcmp(const char*, const char*);
int strncmp(const char*, const char*, size_t);
char* strncpy(char*, const char*, size_t);
char* strcpy(char*, const char*);

char* strcat(char *dest, const char *src);
char* strncat(char *dest, const char *src, size_t n);

char* strchr(const char*, int);

char* strdup( const char* );

#endif

_End_C_Header