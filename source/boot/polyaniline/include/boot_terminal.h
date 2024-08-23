// terminal.h - Header file for terminal output

#ifndef POLY_TERMINAL_H
#define POLY_TERMINAL_H

// Includes
#include <libc_string.h> // String functions

// Functions
#ifdef EFI_PLATFORM
int GOPInit();
#endif


void _clearScreen();
void draw_polyaniline_testTube();
int getX();
int getY();
void setColor(uint8_t color);
int boot_printf(const char * fmt, ...);

#endif