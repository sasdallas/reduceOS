/**
 * @file hexahedron/include/drivers/x86/serial.h
 * @brief x86 serial driver header
 * 
 * Implements a standard for the x86 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_X86_SERIAL_H
#define DRIVERS_X86_SERIAL_H

/**** INCLUDES ****/
#include <stdint.h>

/**** DEFINITIONS ****/

// General
#define SERIAL_CLOCK_RATE       115200

// IO addresses
#define SERIAL_COM1_PORT        0x3F8
#define SERIAL_COM2_PORT        0x2F8
#define SERIAL_COM3_PORT        0x3E8
#define SERIAL_COM4_PORT        0x2E8

// Registers (in terms of IO offsets) (0/1 - DLAB setting)
#define SERIAL_RECEIVE_BUFFER           0   // (0) Receive buffer
#define SERIAL_TRANSMIT_BUFFER          0   // (0) Transmit buffer
#define SERIAL_INTENABLE                1   // (0) Interrupt enable register
#define SERIAL_BAUDRATE_LSB             0   // (1) LSB of baud rate
#define SERIAL_BAUDRATE_MSB             1   // (1) MSB of baud rate
#define SERIAL_IDENTIFICATION           2   // (N) Interrupt identification
#define SERIAL_FIFO_CONTROL             3   // (N) FIFO control register
#define SERIAL_LINE_CONTROL             4   // (N) Line control register
#define SERIAL_MODEM_CONTROL            5   // (N) Modem control register
#define SERIAL_LINE_STATUS              6   // (N) Line status register
#define SERIAL_MODEM_STATUS             7   // (N) Modem status register
#define SERIAL_SCRATCH                  8   // (N) Scratch register

// Bitmasks. Only define what we currently use for now.
// TODO: Define the rest.
#define SERIAL_LINECTRL_DLAB            0x80    // DLAB bit

#define SERIAL_MODEMCTRL_DTR            0x01    // Data transmit ready
#define SERIAL_MODEMCTRL_RTS            0x02    // Ready to send     
#define SERIAL_MODEMCTRL_OUT2           0x08    // Hardware pin OUT2 for IRQs

#define SERIAL_8_DATA                   0x03    // Specify 8 data bits
#define SERIAL_1_STOP                   0x00    // One stop bit
#define SERIAL_NO_PARITY                0x00    // No parity bits

#define SERIAL_FIFO_ENABLE              0x01 | 0x02 | 0x04 // Sets the first 3 bits to enable FIFO and clear receive/transmit


/**** FUNCTIONS ****/

/**
 * @brief Initialize the serial system.
 * 
 * Sets baud rate to whatever's configured in config, enables FIFO,
 * sets up an interface, you know the drill.
 */
int serial_initialize();

/**
 * @brief Changes the serial port baud rate
 * 
 * @param baudrate The baud rate to set
 * @returns -EINVAL on bad baud rate, 0 on success.
 */
int serial_setBaudRate(uint16_t baudrate);

/**
 * @brief Write a character to serial output
 */
int serial_writeCharacter(char ch);

/**
 * @brief Retrieves a character from serial
 */
char serial_receiveCharacter();

#endif