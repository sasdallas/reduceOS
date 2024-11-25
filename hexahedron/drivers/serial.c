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
#include <kernel/mem/alloc.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* Early write character method */
int (*generic_serial_write_character)(char ch) = NULL;

/* Port data */
static serial_port_t *ports[MAX_COM_PORTS];
static serial_port_t *main_port = NULL;

/**
 * @brief Set port
 * @param port The port to set. Depending on the value of COM port it will be added.
 * @param is_main_port Whether this port should be classified as the main port
 * @warning This will overwrite any driver/port already configured
 */
void serial_setPort(serial_port_t *port, int is_main_port) {
    if (!port || port->com_port > MAX_COM_PORTS || port->com_port == 0) return;

    ports[port->com_port - 1] = port;

    if (is_main_port) main_port = port;
}

/**
 * @brief Put character method
 */
int serial_print(void *user, char ch) {
    if (!main_port) {
        // Account for CRLF
        if (ch == '\n') generic_serial_write_character('\r');

        return generic_serial_write_character(ch);
    } else {
        if (ch == '\n') main_port->write(main_port, '\r');

        return main_port->write(main_port, ch);
    }
}

/**
 * @brief Set the serial write method
 */
void serial_setEarlyWriteMethod(int (*write_method)(char ch)) {
    if (write_method) {
        generic_serial_write_character = write_method;
    }
}

/**
 * @brief Exposed serial printing method
 */
int serial_printf(char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int out = xvasprintf(serial_print, NULL, format, ap);
    va_end(ap);
    return out;
}


