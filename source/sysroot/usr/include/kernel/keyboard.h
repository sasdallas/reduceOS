// keyboard.h - header file for keyboard.c, the keyboard driver

#ifndef KEYBOARD_H
#define KEYBOARD_H


// Includes
#include <libk_reduced/stdint.h> // Integer declarations like uint8_t, etc.
#include <libk_reduced/string.h> // String functions
#include <libk_reduced/stdbool.h> // Booleans

#include <kernel/isr.h> // Interrupt Service Routines
#include <kernel/terminal.h> // Terminal functions like printf.
#include <kernel/panic.h> // Register declarations
#include <kernel/heap.h> // Heap declarations (for allocating memory)


// Functions
void keyboardInitialize();                      // keyboardInit() - Registers keyboardHandler on IRQ 33 - enabling the keyboard.
void setKBHandler(bool state);                  // setKBHandler() - Enables / disables the keyboard. Used when setting up the kernel (we don't want those pesky users mucking up our output, do we?)
void keyboardGetLine(char *buffer);             // keyboardGetLine() - Returns keyboard input after ENTER key is pressed.
char keyboardGetChar();                         // keyboardGetChar() - Returns the current character char (after waiting for it to be non-0)
void keyboardGetKey(char key, bool printChars); // keyboardGetKey() - Returns whenever keyboardGetChar returns key (or a special character)
char keyboard_getKeyPressed();                  // keyboard_getKeyPressed() - Returns if a key is being pressed.
bool getControl();                              // getControl() - Returns whether control is down
void keyboard_clearBuffer();                    // keyboard_clearBuffer() - Clear the keyboard buffer
char keyboard_altChars(char ch);                // keyboard_altChars() - Gets alternative characters 

/* BAD FUNCTIONS */
void setKBShiftKey(bool state);                 // setKBShiftKey() - Sets whether the SHIFT key is pressed
void setKBCapsLock(bool state);                 // setKBCapsLock() - Sets whether the CAPS LOCK key is pressed
void setKBCtrl(bool state);                     // setKBCtrl() - Sets whether the CTRL key is pressed
bool getKBShift();                              // getKBShift() - Returns the SHIFT key state
bool getKBCapsLock();                           // getKBCapsLock() - Returns the CAPS LOCK key state
bool getKBCtrl();                               // getKBCtrl() - Returns the CTRL key state

#endif


