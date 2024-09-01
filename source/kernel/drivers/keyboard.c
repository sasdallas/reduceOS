// =============================================
// keyboard.c - reduceOS keyboard driver
// =============================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include <kernel/keyboard.h> // Main include file
#include <libk_reduced/stdio.h>

bool keyboard_enabled = true; // Enabled?
bool keyboard_printChars = true; // Print chars out?
static bool shiftKey = false; // Shift, caps lock, and ctrl key handling.
static bool capsLock = false;
static bool ctrlPressed = false;


char bufferPointer[256];
int bindex = 0;
bool newline = false;


char ch = '\0'; // The char we will output. You may be wondering why it is declared here instead of in keyboardHandler() - that's because a few other functions need access to this variable.


// setKBHandler(bool state) - Changes if the keyboard handler is allowed to save characters.
// Dev notes: Possibly add some special keycodes (like CTRL + C) that can pause the boot process or do something (therefore bypassing this).
void setKBHandler(bool state) { keyboard_enabled = state; }

// setKBPrintChars(bool state) - Changes if the keyboard handler is allowed to output characters.
void setKBPrintChars(bool state) { keyboard_printChars = state; }

// getControl() - Returns whether control is down
bool getControl() { return ctrlPressed; }


// setKBShiftKey(bool state) - Sets the SHIFT key state
void setKBShiftKey(bool state) { shiftKey = state; }

// setKBCapsLock(bool state) - Sets the CAPS LOCK key state
void setKBCapsLock(bool state) { capsLock = state; }

// setKBCtrl(bool state) - Sets the CTRL key state
void setKBCtrl(bool state) { ctrlPressed = state; } 

// getKBShift() - Returns the SHIFT key state
bool getKBShift() { return shiftKey; }

// getKBCapsLock() - Returns the CAPS LOCK key state
bool getKBCapsLock() { return capsLock; }

// getKBCtrl() - Returns the CTRL key state
bool getKBCtrl() { return ctrlPressed; }


char keyboard_altChars(char ch) {
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
    } else if (key == '\b' && bindex > 0) {
        bufferPointer[bindex-1] = '\0';
        bindex--;
    } else {
        bufferPointer[bindex] = key;
        bindex++;
    }
    
}


// keyboardWaitForNewline() - Waits for newline to be pressed.
void keyboardWaitForNewline() {
    while (!newline);
    newline = false;
    
    return;
}


// keyboardGetChar() - Returns the character currently present in ch- only if the character is NOT a \0 (signifying the end)
char keyboardGetChar() {
    char c; // Our return value
    while (true) {
        if (ch == '\0') {    
            //printf(NULL); // UNKNOWN BUG: For some reason, we have to do something whenever this is called, or it doesn't return. I don't know why and I hope this will be fixed later.
        } else {
            c = ch; // Update c and ch to have proper values.
            ch = 0;
            break;
        }
    }
    return c;
}

// Custom method to wait for a character or until when ctrl is pressed, and then it returns -1.
// Bad
char keyboardGetChar_ctrl() {
    char c;
    while (true) {
        if (ch != '\0') {
            c = ch;
            ch = 0;
            return c;
        }

        if (ctrlPressed) {
            return -1;
        }
    }
}

// keyboard_getKeyPressed() - A small method to return the key currently being pressed (if any)
char keyboard_getKeyPressed() {
    return ch;
}


// keyboard_clearBuffer() - Clears the keyboard buffer.
void keyboard_clearBuffer() {
    memset(bufferPointer, 0, sizeof(char) * 256);
    bindex = 0;
}


// keyboardGetKey() - Waits until a specific key is pressed and returns it.
// We leave this one because it can usually keep up.
void keyboardGetKey(char key, bool doPrintChars) {
    bool previousPrintValue = keyboard_printChars; // We will restore this value after we're done.
    setKBPrintChars(doPrintChars); // Set KB print chars to whatever they want.


    // One special key can be passed to this function.
    // If \e is passed, wait for ENTER (ch is '\n').
    // A note about this function: When it says to pass "\e" it means LITERALLY the string 
    // The remaining are deprecated and CANNOT be used.

    if (key == '\e') {
        while (keyboardGetChar() != '\n');
        setKBPrintChars(previousPrintValue);
        return;
    } else {
        while(keyboardGetChar() != key);
        setKBPrintChars(previousPrintValue);
        return;
    }
    
    
}

char *getKeyboardBuffer() {
    return bufferPointer;
}

// keyboardGetLine() - A better version of keyboardGetInput that waits until ENTER key is pressed, and then sends it back (it never actually sends it back, only edits a buffer provided).
void keyboardGetLine(char *buffer) {
    keyboardWaitForNewline();
    strcpy(buffer, bufferPointer);
    keyboard_clearBuffer();
    bindex = 0;
    return;
}



// keyboardInitialize() - Main function that loads the keyboard
void keyboardInitialize() {
    //isrRegisterInterruptHandler(33, keyboardHandler); // Register IRQ 33 as an ISR interrupt handler value.
    printf("Keyboard driver initialized.\n");
}
