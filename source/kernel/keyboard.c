// =============================================
// keyboard.c - reduceOS keyboard driver
// =============================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include "include/keyboard.h" // Main include file

bool isEnabled = true; // As mentioned in keyboard.h, this header file contains definitions for special scancodes and other things.
bool shiftKey = false; // Shift, caps lock, and ctrl key handling.
bool capsLock = false;
bool ctrlPressed = false;

// Keyboard buffer - used with getInput()
char keyboardBuffer[];

// Making life so much easier for me. Instead of manually switching between the scancodes in a switch() statement, just match them to this! So much easier.
const char scancodeChars[] = {
    '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0',
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0'
};



// enableKBHandler(bool state) - Changes if the keyboard handler is allowed to output characters. 
// Dev notes: Possibly add some special keycodes (like CTRL + C) that can pause the boot process or do something (therefore bypassing this).
void enableKBHandler(bool state) { isEnabled = state; }


char altChars(char ch) {
    switch (ch) {
        case '`': return '~';
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case '\\': return '|';
        case ';': return ':';
        case '\'': return '\"';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        default: return ch;
    }
}



// keyboardHandler(REGISTERS *r) - The handler assigned by keyboardInitialize to IRQ 33. Handles all scancode stuff.
static void keyboardHandler(REGISTERS *r) {
    uint8_t scancode = inportb(0x60); // No matter if the handler is enabled or not, we need to read from port 0x60 or the keyboard might stop responding.

    if (!isEnabled) { return; } // Do not continue if not enabled.

    char ch = '\0'; // The char we will output.


    if (scancode & 0x80) {
        // This signifies a key was released. We don't care UNLESS it's the shift key or control key. Then we care.
        if (scancode == 0xAA || scancode == 0xB6) { // Left or right shift key was released
            shiftKey = false;
        }
        
        if (scancode == 0x9D) {
            ctrlPressed = false;
        }
    } else {
        switch (scancode) {
            case SCANCODE_CAPSLOCK:
                if (capsLock) { capsLock = false; }
                else { capsLock = true; }
                break;
            
            case SCANCODE_ENTER:
                ch = '\n';
                break;

            case SCANCODE_LEFTSHIFT:
                shiftKey = true;
                break;

            case SCANCODE_RIGHTSHIFT:
                shiftKey = true;
                break;

            case SCANCODE_CTRL:
                ctrlPressed = true;
                break;

            case SCANCODE_TAB:
                ch = '\t';
                break;
            
            case SCANCODE_LEFT:
                terminalMoveArrowKeys(0);
                break;
            
            case SCANCODE_RIGHT:
                terminalMoveArrowKeys(1);
                break;

            case SCANCODE_SPACE:
                ch = ' ';
                break;
            
            case SCANCODE_BACKSPACE:
                terminalBackspace();
                break;

            default:
                ch = scancodeChars[(int) scancode];
                if (capsLock) {
                    if (shiftKey) { ch = altChars(ch); } // If SHIFT key is also pressed, do nothing except convert to alternate chars.
                    else { ch = toupper(ch); } // Else, convert to upper case.
                } else {
                    if (shiftKey) {
                        if (isalpha(ch)) ch = toupper(ch); 
                        else { ch = altChars(ch); }
                    }
                }
        }
    }
    
    // keyboardGetChar() will handle getting the char and returning it.
    terminalPutchar(ch);
    
    return;
}



// keyboardInitialize() - Main function that loads the keyboard
void keyboardInitialize() {
    isrRegisterInterruptHandler(33, keyboardHandler);
    printf("Keyboard driver initialized.\n");
}



