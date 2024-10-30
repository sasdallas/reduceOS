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

/**** FUNCTIONS ****/

/**
 * @brief Exposed serial printing method
 */
int serial_printf(char *format, ...);

/**
 * @brief Set the serial write method
 */
void serial_setWriteMethod(void (*write_method)(char ch));

#endif