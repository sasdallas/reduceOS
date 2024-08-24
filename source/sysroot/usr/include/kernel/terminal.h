// terminal.h - Handling all terminal functions(like printf, putchar, etc.)


#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h> // size_t declaration
#include <stdint.h> // Integer type declarations
#include <stdbool.h> // Boolean declarations
#include <string.h> // String functions
#include <limits.h> // Limits on integers and more.
#include <stdarg.h>
#include <kernel/graphics.h> // Utility functions
#include <kernel/serial.h> // Serial logging
#include <kernel/vesa.h> // VESA VBE
#include <kernel/vga.h> // VGA functions


// Variable declarations

static size_t terminalX; // X position of terminal buffer, or column
static size_t terminalY; // Y position of terminal buffer, or row
static uint8_t terminalColor; // The current color of the terminal.
static uint16_t *terminalBuffer; // The most important one: the terminal buffer.
extern int vbeWidth, vbeHeight;
extern int terminalMode; // 0 signifies VGA mode, 1 signifies VESA VBE.


// VGA memory address, width, height etc are stored in graphics.h.

// Function declarations

void terminalInit(void); // terminalInit() - load the terminal
void changeTerminalMode(int mode); // changeTerminalMode() - Update the terminal mode (0 = VGA, 1 = VESA)
void updateTerminalColor(uint8_t color); // updateTerminalColor() - Change the terminal color. Requires a vgaColorEntry already setup.
void terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y); // terminalPutcharXY() - Place an unsigned char at a certain X and Y. Not recommended, doesn't include scrolling stuff.
void terminalGotoXY(size_t x, size_t y); // terminalGotoXY() - Change the position to X and Y
void scrollTerminal(); // scrollTerminal() - I don't see how this could be used outside of terminal.c, but just in case. Scrolls the terminal down.
void terminalDeleteLastLine(); // terminalDeleteLastLine() - Removes the last line placed by the terminal.
void clearScreen(uint8_t fg, uint8_t bg); // clearScreen() - Clears the terminal screen.
void terminalPutchar(char c); // terminalPutchar() - Recommended function. Incorporates scrollTerminal and terminalDeleteLastLine.
void terminalWrite(const char *data, size_t size); // terminalWrite() - This nor terminalWriteString is recommended for use. Use printf. It prints data using a for loop, but needs length.
void terminalWriteString(const char* data); // terminalWriteString() - The exact same as terminalWrite but with strlen() included.
void terminalBackspace(); // terminalBackspace() - Removes the last character printed.
void terminalWriteStringXY(const char *data, size_t x, size_t y); // terminalWriteStringXY() - Moves the terminal to X and Y, prints the string, then moves back to the original position.
void terminalMoveArrowKeys(int arrowKey); // terminalMoveArrowKeys() - used by keyboard.c, a function to move the cursor around
void updateBottomText(char *bottomText); // updateBottomText() - A function to update that bottom bar of text
void enableShell(char *shellToUse); // Enables a boundary that cannot be overwritten.
void updateTextCursor_vesa(); // Updating the text cursor in VESA VBE. 
void terminalUpdateScreen(); // Update the screen buffers
void instantUpdateTerminalColor(uint8_t fg, uint8_t bg); // Instantly update the terminal color (VESA only)
void terminalSetUpdateScreen(bool state); // Toggles whether the screen should update when terminalUpdateScreen() is called
void terminalUpdateTopBar(char *text); // Update the top bar on the screen
void terminalUpdateTopBarKernel(char *text); // Updates the top bar and prints out kernel version info
#endif
