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
 * @brief Set the serial write method
 */
void serial_setEarlyWriteMethod(int (*write_method)(char ch));

/**
 * @brief Put character method
 */
int serial_print(void *user, char ch);

/**
 * @brief Exposed serial printing method
 */
int serial_printf(char *format, ...);

#endif