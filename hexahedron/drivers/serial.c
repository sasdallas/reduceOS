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
int (*serial_write_character_early)(char ch) = NULL;

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
 * @brief Returns the port configured
 * @param port The port configured.
 * @returns Either the port or NULL. Whatever's in the list.
 */
serial_port_t *serial_getPort(int port) {
    if (port < 1 || port > MAX_COM_PORTS) return NULL;
    return ports[port-1];
} 

/**
 * @brief Put character method - puts characters to main_port or early write method.
 * @param user Can be put as a serial_port object to write to that, or can be NULL.
 */
int serial_print(void *user, char ch) {
    // If user was specified and not NULL, then we are probably trying to print to a specific port
    if (user) {
        if (ch == '\n') ((serial_port_t*)user)->write((serial_port_t*)user, '\r');
        ((serial_port_t*)user)->write((serial_port_t*)user, ch);
    }

    // Else, do we have a main port?
    if (!main_port) {
        // No, use early write method.
        if (ch == '\n') serial_write_character_early('\r');
        return serial_write_character_early(ch);
    } else {
        // Yes, use that.
        if (ch == '\n') main_port->write(main_port, '\r');
        return main_port->write(main_port, ch);
    }
}

/**
 * @brief Set the serial early write method
 */
void serial_setEarlyWriteMethod(int (*write_method)(char ch)) {
    if (write_method) {
        serial_write_character_early = write_method;
    }
}

/**
 * @brief Serial printing method - writes to main_port
 */
int serial_printf(char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int out = xvasprintf(serial_print, NULL, format, ap);
    va_end(ap);
    return out;
}