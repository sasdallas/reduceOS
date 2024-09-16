// =============================================
// keyboard.c - reduceOS keyboard driver
// =============================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.
// @todo This file sucks

#include <kernel/chardev.h>
#include <kernel/keyboard.h> // Main include file
#include <kernel/vfs.h>
#include <libk_reduced/stdio.h>

bool keyboard_enabled = true;    // Enabled?
bool keyboard_printChars = true; // Print chars out?
static bool shiftKey = false;    // Shift, caps lock, and ctrl key handling.
static bool capsLock = false;
static bool ctrlPressed = false;

static fsNode_t* kbd_dev = NULL;

char kb_buffer[256]; // stack allocated (gross)
int bindex = 0;
bool newline = false;

// The char we will output. You may be wondering why it is declared here instead of in keyboardHandler() - that's because a few other functions need access to this variable.
// They really shouldn't though...
char ch = '\0';

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
        case '`':
            return '~';
        case '1':
            return '!';
        case '2':
            return '@';
        case '3':
            return '#';
        case '4':
            return '$';
        case '5':
            return '%';
        case '6':
            return '^';
        case '7':
            return '&';
        case '8':
            return '*';
        case '9':
            return '(';
        case '0':
            return ')';
        case '-':
            return '_';
        case '=':
            return '+';
        case '[':
            return '{';
        case ']':
            return '}';
        case '\\':
            return '|';
        case ';':
            return ':';
        case '\'':
            return '\"';
        case ',':
            return '<';
        case '.':
            return '>';
        case '/':
            return '?';
        default:
            return ch;
    }
}

// keyboardRegisterKeyPress(char key) - Registers that a key was pressed.
void keyboardRegisterKeyPress(char key) {
    if (key == '\n') {
        newline = true;
    } else if (key == '\b' && bindex > 0) {
        kb_buffer[bindex - 1] = '\0';
        bindex--;
    } else {
        kb_buffer[bindex] = key;
        bindex++;
    }

    if (kbd_dev) {
        // There is a keyboard driver, write to it.
        uint8_t data[] = { key };
        writeFilesystem(kbd_dev, 0, 1, data); // offset is discarded anyways
    }

    if (keyboard_printChars) {
        terminalPutchar(ch);
        vbeSwitchBuffers(); // sorry
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

        if (ctrlPressed) { return -1; }
    }
}

// keyboard_getKeyPressed() - A small method to return the key currently being pressed (if any)
char keyboard_getKeyPressed() { return ch; }

// keyboard_clearBuffer() - Clears the keyboard buffer.
void keyboard_clearBuffer() {
    memset(kb_buffer, 0, sizeof(char) * 256);
    bindex = 0;
}

// keyboardGetKey() - Waits until a specific key is pressed and returns it.
// We leave this one because it can usually keep up.
void keyboardGetKey(char key, bool doPrintChars) {
    bool previousPrintValue = keyboard_printChars; // We will restore this value after we're done.
    setKBPrintChars(doPrintChars);                 // Set KB print chars to whatever they want.

    // One special key can be passed to this function.
    // If \e is passed, wait for ENTER (ch is '\n').
    // A note about this function: When it says to pass "\e" it means LITERALLY the string
    // The remaining are deprecated and CANNOT be used.

    if (key == '\e') {
        while (keyboardGetChar() != '\n');
        setKBPrintChars(previousPrintValue);
        return;
    } else {
        while (keyboardGetChar() != key);
        setKBPrintChars(previousPrintValue);
        return;
    }
}

char* getKeyboardBuffer() { return kb_buffer; }

// keyboardGetLine() - A better version of keyboardGetInput that waits until ENTER key is pressed, and then sends it back (it never actually sends it back, only edits a buffer provided).
void keyboardGetLine(char* buffer) {
    keyboardWaitForNewline();
    strcpy(buffer, kb_buffer);
    keyboard_clearBuffer();
    bindex = 0;
    return;
}

// keyboardInitialize() - Main function that loads the keyboard
void keyboardInitialize() {}

// keyboard_devinit() - Create the keyboard VFS node
void keyboard_devinit() {
    // Grab a chardev for us
    kbd_dev = chardev_create(128, "Keyboard");
    kbd_dev->flags = VFS_CHARDEVICE;
    vfsMount("/device/keyboard", kbd_dev);
}
