/**
 * @file hexahedron/drivers/serial.c
 * @brief Generic serial driver
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/config.h>
#include <kernel/drivers/serial.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/*** PROTOTYPE: BAD IMPLEMENTATION ***/

// Write character method
void (*generic_serial_write_character)(char ch) = NULL;

static int serial_print(void *user, char ch) {
    if (generic_serial_write_character == NULL) return 0; // oh no

    // Account for CRLF
    if (ch == '\n') generic_serial_write_character('\r');

    generic_serial_write_character(ch);
    return 0;
}


/**
 * @brief Set the serial write method
 */
void serial_setWriteMethod(void (*write_method)(char ch)) {
    if (write_method) {
        generic_serial_write_character = write_method;
    }
}


/**
 * @brief Exposed serial printing method
 * @todo Not good
 */
int serial_printf(char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int out = xvasprintf(serial_print, NULL, format, ap);
    va_end(ap);
    return out;
}


