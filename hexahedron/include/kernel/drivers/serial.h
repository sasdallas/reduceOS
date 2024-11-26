/**
 * @file hexahedron/include/kernel/drivers/serial.h
 * @brief Generic serial driver header file
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_SERIAL_H
#define DRIVERS_SERIAL_H

/**** INCLUDES ****/

#include <stdint.h>

/**** TYPES ****/

struct _serial_port;

typedef int (*serial_port_write_t)(struct _serial_port *port, char ch);
typedef char (*serial_port_read_t)(struct _serial_port *port);

/* TODO: Replace this with a VFS node */
typedef struct _serial_port {
    int com_port;               // COM port
    uint32_t baud_rate;         // Baud rate
    
    uint32_t io_address;        // I/O address (for use by driver)

    serial_port_read_t read;    // Read method
    serial_port_write_t write;  // Write method
} serial_port_t;


/**** DEFINITIONS ****/

#define MAX_COM_PORTS 5

/**** FUNCTIONS ****/

/**
 * @brief Set port
 * @param port The port to set. Depending on the value of COM port it will be added.
 * @param is_main_port Whether this port should be classified as the main port
 * @warning This will overwrite any driver/port already configured
 */
void serial_setPort(serial_port_t *port, int is_main_port);

/**
 * @brief Returns the port configured
 * @param port The port configured.
 * @returns Either the port or NULL. Whatever's in the list.
 */
serial_port_t *serial_getPort(int port);

/**
 * @brief Set the serial early write method
 */
void serial_setEarlyWriteMethod(int (*write_method)(char ch));

/**
 * @brief Put character method - puts characters to main_port or early write method.
 * @param user Can be put as a serial_port object to write to that, or can be NULL.
 */
int serial_print(void *user, char ch);

/**
 * @brief Serial printing method - writes to main_port
 */
int serial_printf(char *format, ...);

#endif