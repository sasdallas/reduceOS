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

#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// libk/libc are differing
#ifdef __LIBK 




// Type definitions for xvas_callback
typedef int (*xvas_callback)(void *, char);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/**** FUNCTIONS ****/

/**
 * @brief Put character function
 */
int putchar(int ch);


/**
 * @brief printf() function, by default uses putchar()
 */
int printf(const char * __restrict, ...);

/**
 * @brief Put a string, using putchar()
 */
int puts(const char *);

/**
 * @brief Same as C standard vsnprintf
 */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/**
 * @brief Same as C standard snprintf
 */
int snprintf(char * str, size_t size, const char * format, ...);

/**
 * @brief Same as C standard sprintf
 * @warning Use snprintf(), we need security here!
 */
int sprintf(char * str, const char * format, ...);

/**
 * @brief Same as C standard xvasprintf
 */
size_t xvasprintf(xvas_callback callback, void * userData, const char * fmt, va_list args);

#else


#endif



#endif