/**
 * @file hexahedron/drivers/x86/serial.c
 * @brief x86 serial driver
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <stdint.h>
#include <errno.h>

#include <stdio.h>
#include <stdarg.h>

#include <kernel/drivers/x86/serial.h>
#include <kernel/drivers/serial.h>
#include <kernel/arch/i386/hal.h> // !!!!!! BAD HAL PROBLEM
#include <kernel/config.h>


// Main configured serial port (TODO: Move this to a structure)
uint16_t serial_currentPort = SERIAL_COM1_PORT;
uint16_t serial_currentBaud = 9600;


// Internal function to get the COM port address using configuration info.
static uint16_t serial_getDefaultCOMPort() {
    switch (__debug_com_port) {
        case 1:
            return SERIAL_COM1_PORT;
        case 2:
            return SERIAL_COM2_PORT;
        case 3:
            return SERIAL_COM3_PORT;
        case 4:
            return SERIAL_COM4_PORT;
        default:
            // TODO: Log somehow?
            return SERIAL_COM1_PORT;
    }
}



/**
 * @brief Changes the serial port baud rate
 * 
 * @param baudrate The baud rate to set
 * @returns -EINVAL on bad baud rate, 0 on success.
 */
int serial_setBaudRate(uint16_t baudrate) {
    if (SERIAL_CLOCK_RATE % baudrate != 0) return -EINVAL;

    // Enable the DLAB bit. LSB/MSB registers are not accessible without it
    uint8_t lcr = inportb(serial_currentPort + SERIAL_LINE_CONTROL);
    outportb(serial_currentPort + SERIAL_LINE_CONTROL, lcr | SERIAL_LINECTRL_DLAB);

    // Calculate the divisors and set them
    uint32_t mode = SERIAL_CLOCK_RATE / baudrate;
    outportb(serial_currentPort + SERIAL_BAUDRATE_LSB, (uint8_t)(mode & 0xFF));
    outportb(serial_currentPort + SERIAL_BAUDRATE_MSB, (uint8_t)((mode >> 8) & 0xFF));

    // Reset the DLAB bit
    outportb(serial_currentPort + SERIAL_LINE_CONTROL, lcr);

    // Update baud rate
    serial_currentBaud = baudrate;

    return 0;
}

/**
 * @brief Initialize the serial system.
 * 
 * Sets baud rate to whatever's configured in config, enables FIFO,
 * sets up an interface, you know the drill.
 */
int serial_initialize() {
    // Create basic port data
    serial_currentPort = serial_getDefaultCOMPort();

    // Disable port interrupts
    outportb(serial_currentPort + SERIAL_LINE_CONTROL, 0);
    outportb(serial_currentPort + SERIAL_INTENABLE, 0);
    
    // Enable DTR, RTS, and OUT2
    outportb(serial_currentPort + SERIAL_MODEM_CONTROL,
                SERIAL_MODEMCTRL_DTR | SERIAL_MODEMCTRL_RTS | SERIAL_MODEMCTRL_OUT2);
        
    // Set baud rate
    serial_setBaudRate(__debug_baud_rate);

    // Configure port bit parameters
    outportb(serial_currentPort + SERIAL_LINE_CONTROL,
                SERIAL_8_DATA | SERIAL_1_STOP | SERIAL_NO_PARITY);
    
    // Enable FIFO & clear transmit/receive
    outportb(serial_currentPort + SERIAL_FIFO_CONTROL, SERIAL_FIFO_ENABLE);
    
    // Clear RBR
    inportb(serial_currentPort + SERIAL_RECEIVE_BUFFER);

    // Set ourselves as the write method
    serial_setWriteMethod(serial_writeCharacter);

    return 0;
}







/**
 * @brief Write a character to serial output
 */
void serial_writeCharacter(char ch) {
    // Wait until transmit is empty
    while ((inportb(serial_currentPort + SERIAL_LINE_STATUS) & 0x20) == 0x0);

    // Write character
    outportb(serial_currentPort + SERIAL_TRANSMIT_BUFFER, ch);
}

/**
 * @brief Retrieves a character from serial
 */
char serial_receiveCharacter() {
    // Wait until receive has something
    while ((inportb(serial_currentPort + SERIAL_LINE_STATUS) & 0x01) == 0x0);

    // Return the character
    return inportb(serial_currentPort + SERIAL_RECEIVE_BUFFER);
}