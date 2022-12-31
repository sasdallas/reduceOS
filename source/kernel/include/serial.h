// serial.h - header file for serial.c (serial logging manager)

#ifndef SERIAL_H
#define SERIAL_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/hal.h" // outportb() and inportb()
#include "include/terminal.h" // printf_putchar function.

// Definitions
#define SERIAL_COM1 0x3F8
#define SERIAL_COM2 0x2F8
#define SERIAL_COM3 0x3E8
#define SERIAL_COM4 0x2E8
#define SERIAL_COM5 0x5F8
#define SERIAL_COM6 0x4F8
#define SERIAL_COM7 0x5E8
#define SERIAL_COM8 0x4E8

// Functions
void serialInit(); // Initialize serial (default port COM1, unchangable as of now.)
void serialWrite(char c); // Writes character 'c' to serial when transmit is empty.
void serialPrintf(const char *str, ...); // Prints a formatted line to SERIAL_COM1.
#endif