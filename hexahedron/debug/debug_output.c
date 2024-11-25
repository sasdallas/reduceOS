/**
 * @file hexahedron/debug/debug_output.c
 * @brief Debug log interface
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
#include <kernel/drivers/clock.h>

#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/* TODO: This should be replaced with a VFS node */
static log_putchar_method_t debug_putchar_method = NULL; 


/**
 * @brief Function to print debug string
 */
int debug_print(void *user, char ch) {
    if (!debug_putchar_method) return 0; // Log not yet initialized or failed to initialize correctly.

    /* Account for CRLF. It doesn't hurt any terminals (that I know of) */
    if (ch == '\n') debug_putchar_method('\r');

    return debug_putchar_method(ch);
}

/**
 * @brief Write directly to debug_print
 */
static int debug_write(size_t length, char *buffer) {
    for (size_t i = 0; i < length; i++) {
        debug_print(NULL, buffer[i]);
    }

    return 0;
}

/**
 * @brief dprintf that accepts va_args instead
 */
int dprintf_va(char *module, DEBUG_LOG_TYPE status, char *format, va_list ap) {
    if (status == NOHEADER) goto _write_format;

    // Get the header we want to use
    char header_prefix[5]; // strncpy?
    switch (status) {
        case INFO:
            strncpy(header_prefix, "INFO", 4);
            break;
        case WARN:
            strncpy(header_prefix, "WARN", 4);
            break;
        case ERR:
            strncpy(header_prefix, "ERR ", 4);
            break;
        case DEBUG:
            strncpy(header_prefix, "DBG ", 4);
            break;
        default:
            strncpy(header_prefix, "??? ", 4);
    }

    header_prefix[4] = 0;

    // If the clock driver isn't ready we just print a blank header.
    char header[128];
    size_t header_length = 0;
    if (!clock_isReady()) {
        if (module) {
            header_length = snprintf(header, 128, "[no clock ready] [%s] [%s] ", header_prefix, module);
        } else {
            header_length = snprintf(header, 128, "[no clock ready] [%s] ", header_prefix);
        }
    } else {
        time_t rawtime;
        time(&rawtime);
        struct tm *timeinfo = localtime(&rawtime);
        
        if (module) {
            header_length = snprintf(header, 128, "[%s] [%s] [%s] ", asctime(timeinfo), header_prefix, module);
        } else {
            header_length = snprintf(header, 128, "[%s] [%s] ", asctime(timeinfo), header_prefix);
        }
    }

    debug_write(header_length, header);

_write_format: ;
    int returnValue = xvasprintf(debug_print, NULL, format, ap);

    return returnValue;
}

/**
 * @brief Internal function to print to debug line.
 * 
 * You can call this but it's not recommended. Use dprintf().
 */
int dprintf_internal(char *module, DEBUG_LOG_TYPE status, char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int returnValue = dprintf_va(module, status, format, ap);
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

/**
 * @brief Get the debug putchar method.
 */
log_putchar_method_t debug_getOutput() {
    return debug_putchar_method;
}