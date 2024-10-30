/**
 * @file hexahedron/debug/debug.c
 * @brief Debug/log interface
 * 
 * @note The dprintf() macro is defined in debug.h
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/debug.h>
#include <stdarg.h>
#include <stdio.h>

/* TODO: This should be replaced with a VFS node */
static log_putchar_method_t debug_putchar_method = NULL; 


/**
 * @brief Internal function to print debug string
 */
static int debug_print(void *user, char ch) {
    if (!debug_putchar_method) return 0; // Log not yet initialized or failed to initialize correctly.

    /* Account for CRLF. It doesn't hurt any terminals (that I know of) */
    if (ch == '\n') debug_putchar_method('\r');

    return debug_putchar_method(ch);
}

/**
 * @brief Internal function to print to debug line.
 * 
 * You can call this but it's not recommended. Use dprintf().
 */
int dprintf_internal(char *file, int line, DEBUG_LOG_TYPE status, char *format, ...) {
    if (status == NOHEADER) goto _write_format;



_write_format: ;
    va_list ap;
    va_start(ap, format);
    int returnValue = xvasprintf(debug_print, NULL, format, ap);
    va_end(ap);

    return returnValue;
}

/**
 * @brief Set the debug putchar method
 * @param logMethod A type of putchar method. Returns an integer and takes a character.
 */
void debug_setOutput(log_putchar_method_t logMethod) {
    debug_putchar_method = logMethod;    
}