// keyboard.h - header file for keyboard.c, the keyboard driver

#ifndef KEYBOARD_H
#define KEYBOARD_H

// Includes
#include "include/libc/stdint.h" // Integer declarations like uint8_t, etc.
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/isr.h" // Interrupt Service Routines
#include "include/terminal.h" // Terminal functions like printf.
#include "include/libc/string.h" // String functions
#include "include/panic.h" // Register declarations

// Functions
static void keyboardHandler(REGISTERS *r);
void keyboardInitialize();

#endif