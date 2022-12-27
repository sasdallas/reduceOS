// ===================================================================
// terminal.c - Handles all terminal functions for graphics
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/terminal.h" // This header file contains variable declarations as not to clutter up the actual C code.

static char *shell = "\0"; // This will be used if we're handling typing - mainly used with backspace, to prevent deleting the prompt by accident. If this variable is \0, it will not be used.

// initTerminal() - Load the terminal, setup the buffers, reset the values, etc.
void initTerminal(void) {
    terminalX = 0, terminalY = 0; // Reset terminal X and Y
    terminalColor = vgaColorEntry(COLOR_WHITE, COLOR_CYAN); // The terminal color will be white for the text and cyan for the BG on start.
    terminalBuffer = VIDEO_MEM; // Initialize the terminal buffer as VIDEO_MEM (0xB8000)

    // Now we need to setup the screen by clearing it with ' '.

    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            const size_t index = y * SCREEN_WIDTH + x;
            terminalBuffer[index] = vgaEntry(' ', terminalColor);
        }
    }
}


// updateTerminalColor(uint8_t color) - Update the terminal color. Simple function.
// Why not use gfxColors as the params? Other functions in terminal.c call this, and it's faster to use terminalColor. May change later though.
void updateTerminalColor(uint8_t color) { terminalColor = color; }


// terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y) - Specific function to place a vgaEntry() at a specific point on the screen.
void terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * SCREEN_WIDTH + x; // Same code as in initTerminal(). Calculate index to place char at and copy it.
    terminalBuffer[index] = vgaEntry(c, color);
}

// terminalGotoXY(size_t x, size_t y) - Changes the terminal position to X and Y
void terminalGotoXY(size_t x, size_t y) { 
    terminalX = x;
    terminalY = y;
}




// Unlike the alpha, this one has terminal scrolling!
// scrollTerminal() - Scrolls the terminal
void scrollTerminal() {
    uint16_t blank = 0x20 | (terminalColor << 8);

    if (terminalY >= SCREEN_HEIGHT) {
        int i;
        for (i = 0*SCREEN_WIDTH; i < (SCREEN_HEIGHT-1) * SCREEN_WIDTH; i++) {
            terminalBuffer[i] = terminalBuffer[i+SCREEN_WIDTH];
        }

        for (int i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
            terminalBuffer[i] = blank;
        }

        terminalY = SCREEN_HEIGHT - 1;
    }

    

}


// terminalDeleteLastLine() - delete the last line of the terminal. Useful for scrolling.
void terminalDeleteLastLine() {
    int x, *ptr;

    for (x = 0; x < SCREEN_WIDTH * 2; x++) {
        ptr = VIDEO_MEM + (SCREEN_WIDTH * 2) * (SCREEN_HEIGHT - 1) + x;
        *ptr = 0;
    }
}

// clearScreen(uint8_t color) - clears the screen 
void clearScreen(uint8_t color) {
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            terminalPutcharXY(' ', terminalColor, x, y); // Reset all characters to be ' '. Obviously initTerminal() already does this, but we want to reimplement it without all the setup stuff.
        }
    }

    terminalX = 0; // Reset X & Y values
    terminalY = 0; 
    return;
}

// updateTextCursor() - Updates the text mode cursor to whatever terminalX and terminalY are.
void updateTextCursor() {
    uint16_t pos = terminalY * SCREEN_WIDTH + terminalX;

    outportb(0x3D4, 14);
    outportb(0x3D5, pos >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, pos);
}

// No function description. Proprietary function used only by keyboard driver.
void terminalMoveArrowKeys(int arrowKey) {
    if (arrowKey == 0 && terminalX != 0) {
        terminalGotoXY(terminalX - 1, terminalY);
    } else if (arrowKey == 1 && terminalX != SCREEN_WIDTH) {
        terminalGotoXY(terminalX + 1, terminalY);
    }
}


// terminalPutchar(char c) - This is the recommended function to use (besides printf, that'll be later) as it incorporates scrollTerminal and terminalDeleteLastLine.
void terminalPutchar(char c) {
    int line;
    unsigned char uc = c; // terminalPutcharXY() requires an unsigned char.

    // Perform the scrolling stuff for X
    if (terminalX == SCREEN_WIDTH) {
        terminalX = 0;
        terminalY++;
    }

    // Perform the scrolling stuff for Y
    scrollTerminal();


    // Checking if c is a special character or not.
    if (c == '\n') {
        terminalY++; // Increment terminal Y
        terminalX = 0;
        
    } else if (c == '\b') {
        terminalBackspace(); // Handle backspace
    } else if (c == '\0') {
        // do nothing
    } else if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            terminalPutchar(' ');
        }
    } else {
        terminalPutcharXY(uc, terminalColor, terminalX, terminalY); // Place an entry at terminal X and Y.
        terminalX++;
    }


    

    // Update text mode cursor
    updateTextCursor();
}

// terminalWrite(const char *data, size_t size) - Writes a certain amount of data to the terminal.
void terminalWrite(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) terminalPutchar(data[i]);
}


// terminalWriteString() - The exact same as terminal write, but shorter with no len option (we use strlen). Not recommended for use, use printf (further down in the file)!
void terminalWriteString(const char *data) { terminalWrite(data, strlen(data)); }

// terminalBackspace() - Removes the last character outputed.
void terminalBackspace() {
    if (terminalX == 0) return; // terminalX being 0 would cause a lot of problems.
    if (terminalX <= strlen(shell) && shell != "\0") return; // Cannot overwrite the shell.
    // First, go back one character.
    terminalGotoXY(terminalX-1, terminalY);

    // Then, write a space where the character is.
    terminalPutchar(' ');

    // Finally, set terminalX to one below current.
    terminalGotoXY(terminalX-1, terminalY);
}

void terminalWriteStringXY(const char *data, size_t x, size_t y) {
    size_t previousX = terminalX; // Store terminalX and terminalY in temporary variables
    size_t previousY = terminalY; 

    terminalX = x; // Update terminalX & terminalY
    terminalY = y;

    terminalWriteString(data); // Write the string to the X & Y.

    terminalX = previousX; // Restore terminalX & terminalY to their previous values.
    terminalY = previousY;
}

// updateBottomText() - A kernel function to make handling the beginning graphics easier.
void updateBottomText(char *bottomText) {
    if (strlen(bottomText) > INT_MAX) return -1; // Overflow
    
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY));

    for (int i = 0; i < SCREEN_WIDTH; i++) terminalPutcharXY(' ', vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY), i, SCREEN_HEIGHT - 1);
    terminalWriteStringXY(bottomText, 0, SCREEN_HEIGHT - 1);


    updateTerminalColor(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    return;
}

// enableShell() - Enables a boundary that cannot be overwritten.
void enableShell(char *shellToUse) { shell = shellToUse; }



// *************************************************************************************************
// Some of the following functions are helper functions to printf, as they need to return a value. 
// It is not recommended to use them and they are not included in the header.
// They may be removed and existing functions will be updated to return include values.
// *************************************************************************************************


// putc(int ic) - terminalPutchar with return values.
int putc(int ic) {
    char c = (char) ic;
    terminalWrite(&c, sizeof(c));
    return ic;
}


// print(const char* data, size_t length) - terminalWrite with return values.
static bool print(const char *data, size_t length) {
    const unsigned char* bytes = (const unsigned char*) data;
    for (size_t i = 0; i < length; i++) {
        if (putc(bytes[i]) == EOF) return false;
    }
    return true;
}

// Now we move to the actual main function, printf.

// printf(const char* format, ...) - The most recommended function to use. Don't know why I didn't opt to put this in a libc/ file but whatever. This prints something but has formatting options and multiple arguments.
int printf(const char*  format, ...) {

    if (!format) return 0; // Don't bother if there is no string

    // printf uses VA macros to make handing the ... easier. These are defined in stdarg.h and the va_list is in va_list.h
    va_list args; // Define the arguments list
    va_start(args, format); // Begin reading the arguments to the right of format into args (we will use va_arg to cycle between them.)

    int charsWritten = 0; // This is the value that will be returned on finish

    for (int i = 0; i < strlen(format); i++) {

        switch (format[i]) { // Take the i character of our string format and determine whether it needs special handling.

            case '%': // In the case that it's a percent sign, check what character is after it.

                switch(format[i+1]) {
                    // There are a few possible characters after this, mainly %c, %s, %d, %x, and %i
                    // %c is for characters.
                    case 'c': {
                        char c = va_arg(args, char); // We need to get the next argument that is a character.
                        
                        
                        if (!putc(c)) return -1; // Print error!

                        charsWritten++; // Increment charsWritten.
                        i++; // Increment i.
                        break;
                    }

                    // %s is for strings (or address of).
                    case 's': {
                        const char *c = va_arg(args, const char *); // Char promotes to integer, and we're trying to get the address of the string.
                        char *str = {0}; // The array where the character will go.

                        strcpy(str, (const char *)c); // Copy c to str (dest is the first parameter)
                        
                        if (!print(str, strlen(str))) return -1; // Print error!

                        charsWritten++; // Increment charsWritten
                        i++; // Increment i.
                        break;
                    }
                    
                    // %d or %i is for integers
                    case 'd':
                    case 'i': {
                        int c = va_arg(args, int); // We need to get the next integer in the parameters.
                        char str[32] = {0}; // String buffer where the integer will go.

                        // itoa() takes care of all the negative stuff already.
                        itoa(c, str, 10); // Convert the integer to a string
                        
                        
                        if (!print(str, strlen(str))) return -1; // Print error!

                        charsWritten++; // Increment charsWritten
                        i++; // Increment i
                        break;

                    }

                    // %x or %X is for hexadecimal
                    case 'X':
                    case 'x': {
                        int c = va_arg(args, int); // We need to get the next integer (it will be converted to hexadecimal when we use itoa.) from the parameters.
                        char str[32] = {0}; // String buffer where the hexadecimal will go.

                        // itoa() will convert the integer to base-16 or hex.
                        itoa(c, str, 16); // Convert the integer to a hex string.

                        if (!print(str, strlen(str))) return -1; // Print error!

                        charsWritten++; // Increment charsWritten
                        i++; // Increment i.
                        break;
                    }

                    default:
                        terminalPutchar(format[i]); // If else place the character
                        break;
                }

                break;
            
            default:
                terminalPutchar(format[i]);
                break;

        }
    }

    va_end(args);
    return charsWritten;
} 

