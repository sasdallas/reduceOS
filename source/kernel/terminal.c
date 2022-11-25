// ===================================================================
// terminal.c - Handles all terminal functions for graphics
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/terminal.h" // This header file contains variable declarations as not to clutter up the actual C code.




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

// Unlike the alpha, this one has terminal scrolling (wiki.osdev.org)!
// scrollTerminal(int line) - Scrolls a line downwards.
void scrollTerminal(int line) {
    int *loop;
    char c;

    for (loop = line * (SCREEN_WIDTH * 2) + VIDEO_MEM; loop < SCREEN_WIDTH * 2; loop++) {
        c = *loop;
        *(loop - (SCREEN_WIDTH * 2)) = c;
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

// clearScreen() - clears the screen 
void clearScreen() {
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            terminalPutcharXY(' ', terminalColor, x, y); // Reset all characters to be ' '. Obviously initTerminal() already does this, but we want to reimplement it without all the setup stuff.
        }
    }

    terminalX = 0; // Reset X & Y values
    terminalY = 0; 
    return;
}

// terminalPutchar(char c) - This is the recommended function to use (besides printf, that'll be later) as it incorporates scrollTerminal and terminalDeleteLastLine.
void terminalPutchar(char c) {
    int line;
    unsigned char uc = c; // terminalPutcharXY() requires an unsigned char.


    // Checking if c is a newline or not.
    if (c == '\n') {
        terminalY++; // Increment terminal Y
        terminalX = 0; // Increment terminal X
    } else {
        terminalPutcharXY(uc, terminalColor, terminalX, terminalY); // Place an entry at terminal X and Y.
        terminalX++;
    }


    // Perform the scrolling stuff for X
    if (terminalX == SCREEN_WIDTH) {
        terminalX = 0;
        terminalY++;
    }

    // Perform the scrolling stuff for Y
    if (terminalY == SCREEN_HEIGHT) {
        for (line = 1; line < SCREEN_HEIGHT - 1; line++) {
            scrollTerminal(line);
        }

        terminalDeleteLastLine();
        terminalY = SCREEN_HEIGHT - 1;
    }
}

// terminalWrite(const char *data, size_t size) - Writes a certain amount of data to the terminal.
void terminalWrite(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) terminalPutchar(data[i]);
}


// terminalWriteString() - The exact same as terminal write, but shorter with no len option (we use strlen). Not recommended for use, use printf (further down in the file)!
void terminalWriteString(const char *data) { terminalWrite(data, strlen(data)); }


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
int printf(const char* restrict format, ...) {
    // printf uses VA macros to make handing the ... easier. These are defined in stdarg.h and the va_list is in va_list.h
    va_list args; // Define the arguments list
    va_start(args, format); // Begin reading the arguments to the right of format into args (we will use va_arg to cycle between them.)

    int charsWritten = 0;

    while (*format != '\0') { // We will loop until we see a \0, signifying the end
        size_t maxrem = INT_MAX - charsWritten; // Maximum amount we have until an overflow error occurs.

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') format++;

            size_t amnt = 1;
            while (format[amnt] && format[amnt] != '%') amnt++; // amnt is the amount of characters needed to print before a % ocurrs.

            if (maxrem < amnt) return -1; // If maxrem < amnt, prevent a buffer overflow.
            if (!print(format, amnt)) return -1; // If the print failed, return -1.

            format += amnt; // Increment format to amount (right after the percent)
            charsWritten += amnt; // Increment charsWritten to amount.
        }
        
        // Moving on to percent handling (%c, %s)

        const char* formatBegunAt = format++;

        if (*format == 'c') {
            format++; // Increment format
            char c = (char) va_arg(args, int); // (char promotes to int) Get the next variable in the arguments.

            if (!maxrem) return -1; // Overflow error!
            if (!print(&c, sizeof(c))) return -1; // Print error!

            charsWritten++; // Increment charsWritten
        } else if (*format == 's') {
            format++;
            const char* str = va_arg(args, const char*); // Get the next variable in the arguments (in this case, a string).
            size_t len = strlen(str); // Get the length of the string
            
            if (maxrem < len) return -1; // Overflow error!
            if (!print(str, len)) return -1; // Print error!
            
            charsWritten += len; // Increment charsWritten by length.
        } else {
            format = formatBegunAt;
            size_t len = strlen(format);

            if (maxrem < len) return -1; // Overflow error!
            if (!print(format, len)) return -1; // Print error!

            charsWritten += len; // Increment charsWritten by length
            format += len; // Increment format by length
        }

    }

    va_end(args);
    return charsWritten;
} 


