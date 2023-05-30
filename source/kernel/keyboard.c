// =============================================
// keyboard.c - reduceOS keyboard driver
// =============================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include "include/keyboard.h" // Main include file

static bool isEnabled = true; // As mentioned in keyboard.h, this header file contains definitions for special scancodes and other things.
static bool shiftKey = false; // Shift, caps lock, and ctrl key handling.
static bool capsLock = false;
static bool ctrlPressed = false;
static bool printChars = true;


char *bufferPointer = NULL;
int bindex = 0;
bool newline = false;

// Making life so much easier for me. Instead of manually switching between the scancodes in a switch() statement, just match them to this! So much easier.
const char scancodeChars[] = {
    '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '\0',
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0'
};

char ch = '\0'; // The char we will output. You may be wondering why it is declared here instead of in keyboardHandler() - that's because a few other functions need access to this variable.


// setKBHandler(bool state) - Changes if the keyboard handler is allowed to save characters.
// Dev notes: Possibly add some special keycodes (like CTRL + C) that can pause the boot process or do something (therefore bypassing this).
void setKBHandler(bool state) { isEnabled = state; }

// setKBPrintChars(bool state) - Changes if the keyboard handler is allowed to output characters.
void setKBPrintChars(bool state) { printChars = state; }

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



// keyboardRegisterKeyPress(char key) - Registers that a key was pressed.
void keyboardRegisterKeyPress(char key) {

    if (key == '\n') {
        newline = true;
    } else if (key == '\b' && bindex != 0) {
        bufferPointer[bindex-1] = '\0';
        bindex--;
    } else {
        bufferPointer[bindex] = key;
        bindex++;
    }
    
}

void keyboardWaitForNewline() {
    while (!newline);
    newline = false;
    
    return;
}

// keyboardHandler(registers_t *r) - The handler assigned by keyboardInitialize to IRQ 33. Handles all scancode stuff.
static void keyboardHandler(registers_t *r) {
    uint8_t scancode = inportb(0x60); // No matter if the handler is enabled or not, we need to read from port 0x60 or the keyboard might stop responding.

    if (!isEnabled) { return; } // Do not continue if not enabled.
   

    if (scancode & 0x80) {
        // This signifies a key was released. We don't care UNLESS it's the shift key or control key. Then we care.
        if (scancode == 0xAA || scancode == 0xB6) { // Left or right shift key was released
            shiftKey = false;
        }
        
        if (scancode == 0x9D) {
            ctrlPressed = false;
        }
        ch = '\0';
    } else {
        switch (scancode) {
            case SCANCODE_CAPSLOCK:
                ch = '\0';
                // I have no idea why this works. It just does.
                // Otherwise the system automatically enables capslock and screws up the command parser.
                // Again: NO. IDEA.

                printf("\0\0\0\0\0\0\0\0");
                if (capsLock) { capsLock = false; }
                else {
                    capsLock = true;
                }
                break;
            
            case SCANCODE_ENTER:
                ch = '\n';
                break;

            case SCANCODE_LEFTSHIFT:
                ch = '\0';
                shiftKey = true;
                break;

            case SCANCODE_RIGHTSHIFT:
                ch = '\0';
                shiftKey = true;
                break;

            case SCANCODE_CTRL:
                ch = '\0';
                ctrlPressed = true;
                break;

            case SCANCODE_TAB:
                ch = '\t';
                break;
            
            case SCANCODE_LEFT:
                ch = '\0';
                terminalMoveArrowKeys(0);
                break;
            
            case SCANCODE_RIGHT:
                ch = '\0';
                terminalMoveArrowKeys(1);
                break;

            case SCANCODE_SPACE:
                ch = ' ';
                break;
            
            case SCANCODE_BACKSPACE:
                ch = '\b'; // \b will tell terminalPutchar() to handle this.
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
    
    // keyboardGetChar() will handle getting the char and returning it (for the shell).
    // Before we return we need to report that the key was pressed.
    if (ch <= 0 || ch == '\0') {
        // Do nothing if ch is 0 or \n. 
    } else {
        keyboardRegisterKeyPress(ch);
        if (printChars) terminalPutchar(ch);
    }
    
    return;
}


// keyboardGetChar() - Returns the character currently present in ch - only if the character is NOT a \0 (signifying the end)
char keyboardGetChar() {
    char c; // Our return value
    while (true) {
        if (ch == '\0') {    
            printf(NULL); // UNKNOWN BUG: For some reason, we have to do something whenever this is called, or it doesn't return. I don't know why and I hope this will be fixed later.
        } else {
            c = ch; // Update c and ch to have proper values.
            ch = 0;
            break;
        }
    }
    return c;
}

// isKeyPressed() - A small method to return the key currently being pressed (if any)
char isKeyPressed() {
    return ch;
}


// clearBuffer() - Clears the keyboard buffer.
void clearBuffer() {
    memset(bufferPointer, 0, sizeof(char) * 256);
}


// keyboardGetKey() - Waits until a specific key is pressed and returns it.
// We leave this one because it can usually keep up.
void keyboardGetKey(char key, bool doPrintChars) {
    bool previousPrintValue = printChars; // We will restore this value after we're done.
    setKBPrintChars(doPrintChars); // Set KB print chars to whatever they want.


    // A few special keys can be passed to this function.
    // If \e is passed, wait for ENTER (ch is '\n'). If \s is passed, wait for SHIFT (shiftKey is true.).
    // If \c is passed, wait for CTRL (ctrlPressed is true).

    if (key == '\e') {
        while (keyboardGetChar() != '\n');
        setKBPrintChars(previousPrintValue);
        return;
    } else if (key == '\s') {
        while (!shiftKey);
        setKBPrintChars(previousPrintValue);
        return;
    } else if (key == '\c') {
        while (!ctrlPressed);
        setKBPrintChars(previousPrintValue);
        return;
    } else {
        while(keyboardGetChar() != key);
        setKBPrintChars(previousPrintValue);
        return;
    }
    
    
}


// keyboardGetLine() - A better version of keyboardGetInput that waits until ENTER key is pressed, and then sends it back (it never actually sends it back, only edits a buffer provided).
void keyboardGetLine(char *buffer) {
    keyboardWaitForNewline();
    strcpy(buffer, bufferPointer);
    clearBuffer();
    bindex = 0;
    return;
}


// keyboardInitialize() - Main function that loads the keyboard
void keyboardInitialize() {
    bufferPointer = kmalloc(8);
    isrRegisterInterruptHandler(33, keyboardHandler); // Register IRQ 33 as an ISR interrupt handler value.
    printf("Keyboard driver initialized.\n");
}