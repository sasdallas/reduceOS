/**
 * @file libpolyhedron/include/stdio.h
 * @brief Standard I/O header file
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

#ifndef _STDIO_H
#define _STDIO_H



#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**** TYPES ****/

// Type definitions for xvas_callback
typedef int (*xvas_callback)(void *, char);


/**** DEFINITIONS ****/

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/**** FUNCTIONS ****/

int putchar(int ch);
int printf(const char * __restrict, ...);
int puts(const char *);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char * str, size_t size, const char * format, ...);
int sprintf(char * str, const char * format, ...);
size_t xvasprintf(xvas_callback callback, void * userData, const char * fmt, va_list args);



#endif

_End_C_Header