#ifndef PS2_MOD_H
#define PS2_MOD_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/keyboard.h>
#include <kernel/isr.h>


// PS/2 Controller Definitions
#define PS2_DATA_PORT               0x60 // R/W
#define PS2_STATUS_PORT             0x64 // R
#define PS2_CMD_PORT                0x64 // W

// PS/2 status bitflags
#define PS2_STATUS_OUTPUTBUF        0x01 // Output buffer status (0 = empty, 1 = full) 
#define PS2_STATUS_INPUTBUF         0x02 // Input buffer status (0 = empty, 1 = full)
#define PS2_STATUS_SYSFLAG          0x04 // System flag, meant to be cleared and reset by firmware (we can't trust the BIOS!!!!)
#define PS2_STATUS_CMDDATA          0x08 // Command/data (0 = data written to input buffer is data for the device, 1 = data is for the controller)
#define PS2_STATUS_TIMEOUT          0x40 // Timeout error (if 1)
#define PS2_STATUS_PARITY           0x80 // Parity error (if 1)

// PS/2 commands
#define PS2_COMMAND_GETCCB          0x20 // Read byte 0 from internal RAM
#define PS2_COMMAND_GETBYTE         0x21 // 0x21 - 0x3F, read byte N from controler RAM
#define PS2_COMMAND_WRITECCB        0x60 // Write byte 0 to internal RAM
#define PS2_COMMAND_WRITEBYTE       0x61 // 0x61 - 0x6F, write byte to controller RAM
#define PS2_COMMAND_DISABLE2        0xA7 // Disable second PS/2 port
#define PS2_COMMAND_ENABLE2         0xA8 // Enable second PS/2 port
#define PS2_COMMAND_TEST2           0xA9 // Test second PS/2 port
#define PS2_COMMAND_TEST            0xAA // Test the controller
#define PS2_COMMAND_TEST1           0xAB // Test the first PS/2 port
#define PS2_COMMAND_DIAG            0xAC // Diagnostic dump (read all bytes of internal RAM - controller specific!)
#define PS2_COMMAND_DISABLE1        0xAD // Disable the first PS/2 port
#define PS2_COMMAND_ENABLE1         0xAE // Enable the first PS/2 port
#define PS2_COMMAND_GETINPUT        0xC0 // Get the controller input port
#define PS2_COMMAND_COPYSTATUS03    0xC1 // Copy bits 0-3 of input port to status bits 4-7
#define PS2_COMMAND_COPYSTATUS47    0xC2 // Copy bits 4-7 of input port to status bits 4-7
#define PS2_COMMAND_READOUTPUT      0xD0 // Read the controller output port
#define PS2_COMMAND_WRITEOUTPUT     0xD1 // Write to the controller output port
#define PS2_COMMAND_WRITEOUTPUT1    0xD2 // Write next byte to controller output port #1 (check if buffer is empty first!)
#define PS2_COMMAND_WRITEOUTPUT2    0xD3 // Write next byte to controller output port #2 (check if buffer is empty first!)
#define PS2_COMMAND_WRITEINPUT      0xD4 // Write next byte to controller input port #2
#define PS2_COMMAND_PULSEOUTPUT     0xF0 // This command uses bits 0-3 as a mask (0 = pulse line, 1 = don't pulse line) 


// CCB flags (only what we need)
#define PS2_PORT1_INTERRUPT         0x01
#define PS2_PORT2_INTERRUPT         0x02


// Typedefs

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

#define MAX_BUFFER_CHARS 500    // We can hold about 500 chars - this is a hard limit (as of now). TODO: Possibly, if it turns out we do need to have 500 chars as a hard limit, add a speaker beep that will notify the user when the buffer is overflowing.




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
void ps2_kbd_init();

#endif