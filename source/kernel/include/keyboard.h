// keyboard.h - header file for keyboard.c, the keyboard driver

#ifndef KEYBOARD_H
#define KEYBOARD_H


// Includes
#include "include/libc/stdint.h" // Integer declarations like uint8_t, etc.
#include "include/libc/string.h" // String functions
#include "include/libc/stdbool.h" // Booleans

#include "include/isr.h" // Interrupt Service Routines
#include "include/terminal.h" // Terminal functions like printf.
#include "include/panic.h" // Register declarations


// Definitions

#define SCANCODE_ESC 0x01
#define SCANCODE_BACKSPACE 0x0E
#define SCANCODE_ENTER 0x1C
#define SCANCODE_CTRL 0x1D // CTRL key is special (along with a few others) - The keyboard signifies left or right control based on whether the extended byte (E0) was sent first. We don't take into account extended bit - we don't need to! There is no difference between right and left control (the kb handler picks both up!)
#define SCANCODE_LEFTSHIFT 0x2A 
#define SCANCODE_RIGHTSHIFT 0x36
#define SCANCODE_ALT 0x38
#define SCANCODE_CAPSLOCK 0x3A
#define SCANCODE_F1 0x3B
#define SCANCODE_F2 0x3C
#define SCANCODE_F3 0x3D
#define SCANCODE_F4 0x3E
#define SCANCODE_F5 0x3F
#define SCANCODE_F6 0x40
#define SCANCODE_F7 0x41
#define SCANCODE_F8 0x42
#define SCANCODE_F9 0x43
#define SCANCODE_F10 0x44
#define SCANCODE_NUMLOCK 0x45
#define SCANCODE_SCROLL_LOCK 0x46
#define SCANCODE_HOME 0x47
#define SCANCODE_UP 0x18
#define SCANCODE_PGUP 0x49
#define SCANCODE_DOWN 0x19
#define SCANCODE_PGDOWN 0x51
#define SCANCODE_LEFT 0x38
#define SCANCODE_RIGHT 0x45
#define SCANCODE_F11 0x57
#define SCANCODE_F12 0x58
#define SCANCODE_TAB 0x0F
#define SCANCODE_SPACE 0x39
#define SCANCODE_EXTENDEDBYTE 0xE0

#define MAX_BUFFER_CHARS 256    // We can hold about 256 chars - this is a hard limit (as of now). TODO: Possibly, if it turns out we do need to have 256 chars as a hard limit, add a speaker beep that will notify the user when the buffer is overflowing.




// Typedefs

typedef enum scancodes_special {
    DETECTION_ERROR = 0x00, // This can also mean an internal buffer overrun
    SELF_TEST_PASS = 0xAA, // Sent after reset (0xFF) command, or kb power up.
    ECHO_RESP = 0xEE, // Sent after echo command (0xEE)
    CMD_ACK = 0xFA, // Sent when a command is acknowledged.
    SELF_TEST_FAIL1 = 0xFC, // Sent when the keyboard fails a self test. Sent after reset (0xFF) command or kb power up. Note there are two possible values sent.
    SELF_TEST_FAIL2 = 0xFD,
    RESEND_CMD = 0xFE, // Keyboard wants control or to repeat last command
    DETECTION_ERROR2 = 0xFF // Sent on a detection error (or internal buffer overrun)
};

typedef enum LEDStates {
    ScrollLock = 0,
    NumberLock = 1,
    CapsLock = 2
};


// Functions
static void keyboardHandler(REGISTERS *r); // keyboardHandler() - Identifies scancodes, printing, etc. Should NEVER be called outside of isrIRQHandler (REGISTERS are passed from the assembly code.)
void keyboardInitialize(); // keyboardInit() - Registers keyboardHandler on IRQ 33 - enabling the keyboard.
void enableKBHandler(bool state); // enableKBHandler() - Enables / disables the keyboard. Used when setting up the kernel (we don't want those pesky users mucking up our output, do we?)
void keyboardGetLine(char *buffer, size_t bufferSize); // keyboardGetLine() - Returns keyboard input after ENTER key is pressed.
char keyboardGetChar(); // keyboardGetChar() - Returns the current character char (after waiting for it to be non-0)
#endif