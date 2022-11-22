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


// terminalPutchar(char c) - This is the recommended function to use (besides printf, that'll be later) as it incorporates scrollTerminal and terminalDeleteLastLine.
void terminalPutchar(char c) {
    int line;
    unsigned char uc = c; // terminalPutcharXY() requires an unsigned char.

    terminalPutcharXY(uc, terminalColor, terminalX, terminalY); // Place an entry at terminal X and Y.

    // Perform the scrolling stuff
    if (++terminalX == SCREEN_WIDTH) {
        terminalX = 0;
        if (++terminalY == SCREEN_HEIGHT) {
            for (line = 1; line <= SCREEN_HEIGHT - 1; line++) { scrollTerminal(line); }
            terminalDeleteLastLine();
            terminalY = SCREEN_HEIGHT - 1;
        }
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
        if (putc(bytes[i]) == -1) return false;
    }
    return true;
}

// Now we move to the actual main function, printf.

// printf(const char* format, ...) - The most recommended function to use. Don't know why I didn't opt for a libc file but whatever. This prints something but has formatting options and multiple arguments.
int printf(const char* restrict format, ...) {
    int charsWritten = 0;
    char **arg = (char **)&format; // This references the arguments in printf. It will increment to go to the next argument (as we don't have a name for it)

    while (*format != '\0') { // Continue the while loop until c = \0, signaling the end of the string.
        size_t maxrem = INT_MAX - charsWritten; // maxrem helps prevent overflow errors (for security and for bugs)

        // Handling percents in the string. Following if statements are about %s, %c, etc.
        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') format++;
            size_t amnt = 1;
            while (format[amnt] && format[amnt] != '%') amnt++;

            if (maxrem < amnt) return -1; // Overflow
            if (!print(format, amnt)) return -1; // Print error.

            format += amnt;
            charsWritten += amnt;
            continue;
        }
        
        const char* formatStartsAt = format++;

        
        if (*format == 'c') { // Taking care of %c
            format++;
            char c = (char) arg++;
            if (!maxrem) return -1; // Overflow
            if (!print(&c, sizeof(c))) return -1; // Print error.

        } else if (*format == 's') { // Taking care of %s
            format++;
            const char* str = *arg++;
            size_t len = strlen(str);

            if (maxrem < len) return -1; // Overflow
            if (!print(str, len)) return -1; // Print error.

        } else { // If there are no modifiers at all.
            format = formatStartsAt;
            size_t len = strlen(format);
            
            if (maxrem < len) return -1; // Overflow
            if (!print(format, len)) return -1; // Print error.

            charsWritten += len;
            format += len;
        }
        
    }

    return charsWritten;
} 


