/**
 * @file hexahedron/include/kernel/debug.h
 * @brief Debugger header file
 * 
 * Provides interface to kernel debugger and logger. 
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

/**** INCLUDES ****/
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/**** TYPES ****/
typedef int (*log_putchar_method_t)(char ch); // Put character method used by logger

typedef enum {
    NOHEADER = 0,       // Do not use any header, including file/timestamp/etc. This is mainly used for some "cool" formatting.
    INFO = 1,           // Prefix with [INFO]
    WARN = 2,           // Prefix with [WARN]
    ERR = 3,            // Prefix with [ERR ]
    DEBUG = 4,          // Prefix with [DBG ]
} DEBUG_LOG_TYPE;

/**** DEFINITIONS ****/

// NOTE: These colors won't actually be used by dprintf. You have to manually specify them.
#define INFO_COLOR_CODE     "\033[36m\033[36m"
#define WARN_COLOR_CODE     "\033[33m\033[33m"
#define ERR_COLOR_CODE      "\033[31m\033[31m"
#define DEBUG_COLOR_CODE    "\033[37m\033[37m"


/**** FUNCTIONS ****/

/**
 * @brief Print something to the debug log
 * 
 * @param status Whether this is debugging information, normal information, errors, or warnings. @see DEBUG_LOG_TYPE
 * @param format The text to print
 */

#ifdef __INTELLISENSE__
// I hate intellisense. Make it look nice! (quick hack)
void dprintf(DEBUG_LOG_TYPE status, char *fmt, ...);
#else
#define dprintf(status, format, ...) dprintf_internal(__FILE__, __LINE__, status, format, ## __VA_ARGS__)
#endif

/**
 * @brief Internal function to print to debug line.
 * 
 * You can call this but it's not recommended. Use dprintf().
 */
int dprintf_internal(char *file, int line, DEBUG_LOG_TYPE status, char *format, ...);

/**
 * @brief Set the debug putchar method
 * @param logMethod A type of putchar method. Returns an integer and takes a character.
 */
void debug_setOutput(log_putchar_method_t logMethod);



#endif