// terminal.h - Handling all terminal functions(like printf, putchar, etc.)


#ifndef TERMINAL_H
#define TERMINAL_H

#include "include/libc/stddef.h" // size_t declaration
#include "include/libc/stdint.h" // Integer type declarations
#include "include/libc/stdbool.h" // Boolean declarations
#include "include/libc/string.h" // String functions
#include "include/libc/limits.h" // Limits on integers and more.
#include "include/libc/stdarg.h" // va argument handling (for ... on printf)
#include "include/libc/va_list.h" // va_list declared here.
#include "include/graphics.h" // Utility functions

// Variable declarations

static size_t terminalX; // X position of terminal buffer, or column
static size_t terminalY; // Y position of terminal buffer, or row
static uint8_t terminalColor; // The current color of the terminal.
static uint16_t *terminalBuffer; // The most important one: the terminal buffer.


// VGA memory address, width, height etc are stored in graphics.h.

// Function declarations

void terminalInit(void); // terminalInit() - load the terminal
void updateTerminalColor(uint8_t color); // updateTerminalColor() - Change the terminal color. Requires a vgaColorEntry already setup.
void terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y); // terminalPutcharXY() - Place an unsigned char at a certain X and Y. Not recommended, doesn't include scrolling stuff.
void terminalGotoXY(size_t x, size_t y); // terminalGotoXY() - Change the position to X and Y
void scrollTerminal(); // scrollTerminal() - I don't see how this could be used outside of terminal.c, but just in case. Scrolls the terminal down.
void terminalDeleteLastLine(); // terminalDeleteLastLine() - Removes the last line placed by the terminal.
void clearScreen(uint8_t color); // clearScreen() - Clears the terminal screen.
void terminalPutchar(char c); // terminalPutchar() - Recommended function. Incorporates scrollTerminal and terminalDeleteLastLine.
void terminalWrite(const char *data, size_t size); // terminalWrite() - This nor terminalWriteString is recommended for use. Use printf. It prints data using a for loop, but needs length.
void terminalWriteString(const char* data); // terminalWriteString() - The exact same as terminalWrite but with strlen() included.
void terminalBackspace(); // terminalBackspace() - Removes the last character printed.
void terminalWriteStringXY(const char *data, size_t x, size_t y); // terminalWriteStringXY() - Moves the terminal to X and Y, prints the string, then moves back to the original position.
void terminalMoveArrowKeys(int arrowKey); // terminalMoveArrowKeys() - used by keyboard.c, a function to move the cursor around
void updateBottomText(char *bottomText); // updateBottomText() - A function to update that bottom bar of text
int printf(const char* restrict format, ...); // printf() - the main function of the entire file. Handles unlimited arguments, %s and %c, and scrolling.
void debugterminalPutchar(char c); // REMOVE
// As described in the C file, certain printf functions are NOT present in this file, like putc and print, as they are only helper functions for printf.
// They are likely going to be removed and merged into terminal.c.
#endif