// ===================================================================
// terminal.c - Handles all terminal functions for graphics
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/terminal.h" // This header file contains variable declarations as not to clutter up the actual C code.

static char *shell = "\0"; // This will be used if we're handling typing - mainly used with backspace, to prevent deleting the prompt by accident. If this variable is \0, it will not be used.


// The terminal's mode is either VGA or VESA VBE. VESA VBE is currently using PSF functions (todo: implement a system wide font API to allow for changing between different fonts)
int vbeTerminalForeground, vbeTerminalBackground;
int terminalMode; // 0 signifies VGA mode, 1 signifies VESA VBE.
int vbeWidth, vbeHeight;

// initTerminal() - Load the terminal, setup the buffers, reset the values, etc.
void initTerminal(void) {
    if (!terminalMode) {
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
    } else {
        // VBE reinit code goes here
        terminalX = 0, terminalY = 0; // Reset X and Y
        vbeTerminalForeground = COLOR_WHITE;
        vbeTerminalBackground = COLOR_CYAN;
        vbeWidth = modeWidth;
        vbeHeight = modeHeight;
    }
}

// changeTerminalMode(int mode) - Changes the mode of the terminal, VGA or VBE.
void changeTerminalMode(int mode) { terminalMode = mode; initTerminal(); }

// updateTerminalColor(uint8_t color) - Update the terminal color. Simple function.
// ! OBSOLETE AND TO BE REMOVED !
void updateTerminalColor(uint8_t color) { terminalColor = color; }

// updateTerminalColor_gfx(uint8_t fg, uint8_t bg)
void updateTerminalColor_gfx(uint8_t fg, uint8_t bg) {
    vbeTerminalForeground = fg;
    vbeTerminalBackground = bg;
    terminalColor = vgaColorEntry(fg, bg);
}


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


// This might seem bad, but it's actually not the worst.
extern uint32_t *framebuffer;
extern int psfGetFontHeight();

// scrollTerminal_vesa() - I wonder what this function does.
void scrollTerminal_vesa() {
    if (terminalY >= modeHeight) {
        
        // Reading directly from vmem is very slow, but reading from our secondary buffer is not
        for (int y = psfGetFontHeight(); y < modeHeight; y++) {
            for (int x = 0; x < modeWidth; x++) {
                vbePutPixel(x, y - psfGetFontHeight(), *(framebuffer + (y*modeWidth + x))); // Copy the pixels one line below to the current line.
            }    
        }

        // Blank the bottom line.
        for (int y = modeHeight-psfGetFontHeight(); y < modeWidth; y++) {
            for (int x = 0; x < modeWidth; x++) {
                vbePutPixel(x, y, VGA_TO_VBE(vbeTerminalBackground));
            }
        }


        // Update terminalY to the new position after scrolling
        terminalY = modeHeight - psfGetFontHeight();
    }
}


// Unlike the alpha, this one has terminal scrolling!
// scrollTerminal() - Scrolls the terminal
void scrollTerminal() {
    if (terminalMode == 1) {
        scrollTerminal_vesa();
        return;
    }

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

void clearScreen_VESA(uint8_t fg, uint8_t bg) {
    updateTerminalColor_gfx(fg, bg);

    terminalX = 0;
    terminalY = 0;


    for (size_t y = 0; y < vbeHeight; y++) {
        for (size_t x = 0; x < vbeWidth; x++) {
            // bad code fix later
            vbePutPixel(x, y, VGA_TO_VBE(vbeTerminalBackground));
        }
    }
    vbeSwitchBuffers();

    terminalX = 0;
    terminalY = 0;
}

// clearScreen(uint8_t fg, uint8_t bg) - clears the screen 
void clearScreen(uint8_t fg, uint8_t bg) {

    if (terminalMode) {
        clearScreen_VESA(fg, bg);
        return;
    }

    terminalColor = vgaColorEntry(fg, bg);

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


// updateTextCursor_vesa() - Does the same thing but in VESA (aka much more complex) - event in PIT IRQ

static int blinkTime = 0; // Time since last blink (~500ms)
static int blinkedLast = 0; // Did it blink last time? Do we need to clear/draw it?
void updateTextCursor_vesa() {
    // This function is called by pitTicks on a reasonable level
    if (blinkTime > 500) {
        if (blinkedLast) {
            for (int x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
                vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(vbeTerminalBackground));
            }
            blinkedLast = 0;
        } else {
            for (int x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
                vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(vbeTerminalForeground));
            }
            blinkedLast = 1;
        }

        vbeSwitchBuffers();
        blinkTime = 0;
    } else {
        blinkTime++;
    }
}



// When a character is typed, we want the cursor to follow it.
static void redrawTextCursor_vesa() {
    if (blinkedLast) {
        blinkTime = 0;
        for (int x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
                vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(vbeTerminalForeground));
        }    
    }
}

// If a newline is typed, remove the text cursor if it existed.
static void clearTextCursor_vesa() {
    if (blinkedLast) {
        for (int x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
            vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(vbeTerminalBackground));
        }
    }
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
    if (terminalMode == 1) {
        terminalPutcharVESA(c);
        return;
    }
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
    } else if (c == '\r') {
        terminalX = 0;  
    } else {
        terminalPutcharXY(uc, terminalColor, terminalX, terminalY); // Place an entry at terminal X and Y.
        terminalX++;
    }


    

    // Update text mode cursor
    updateTextCursor();
}

// terminalPutcharVESA(char c) - VESA VBE putchar method (using PSF as of now)
// Note: Don't call this directly! vbeSwitchBuffers is not called, could result in issues and annoyances later.
void terminalPutcharVESA(char c) {
    int line;
    unsigned char uc = c;

    if (terminalX == vbeWidth) {
        terminalY = terminalY + psfGetFontHeight();
        terminalX = 0;
    }

    if (c == '\n') {
        clearTextCursor_vesa(); // Clear the text cursor.
        terminalY = terminalY + psfGetFontHeight();
        terminalX = 0;
    } else if (c == '\b') {
        clearTextCursor_vesa(); // Clear the text cursor.
        terminalBackspaceVESA();
    } else if (c == '\0') {
        // Do nothing
    } else if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            terminalPutcharVESA(' ');
        }
    } else if (c == '\r') {
        terminalX = 0;
    } else {
        psfDrawChar(c, terminalX, terminalY, VGA_TO_VBE(vbeTerminalForeground), VGA_TO_VBE(vbeTerminalBackground));
        terminalX = terminalX + psfGetFontWidth(); 
    }

    scrollTerminal_vesa();
    
}

// terminalWrite(const char *data, size_t size) - Writes a certain amount of data to the terminal.
void terminalWrite(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminalPutchar(data[i]); 
    }

    // printf() will handle the terminalPutchar
}


// terminalWriteString() - The exact same as terminal write, but shorter with no len option (we use strlen). Not recommended for use, use printf (further down in the file)!
void terminalWriteString(const char *data) { terminalWrite(data, strlen(data)); }

// terminalBackspace() - Removes the last character outputted.
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

// terminalBackspaceVESA() - Removes the last character outputted (VBE mode)
void terminalBackspaceVESA() {
    if (terminalX < psfGetFontWidth()) return;
    if (terminalX <= strlen(shell)*psfGetFontWidth() && shell != "\0") return; // Cannot overwrite shell

    // Go back one character.
    terminalX = terminalX - psfGetFontWidth();
    
    // Then, write a space where the character is.
    terminalPutcharVESA(' ');

    // Set terminalX to one behind current.
    terminalX = terminalX - psfGetFontWidth();
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


// instantUpdateTerminalColor(uint8_t fg, uint8_t bg) - Instantly update the terminal color (VESA only)
void instantUpdateTerminalColor(uint8_t fg, uint8_t bg) {
    if (terminalMode != 1) return; // VESA only

    // NOTE: Function breaks if it tries to update a section where fg and bg are the same.
    // The way we do this is to iterate over the entire thing two times - once to update bg, once to update fg.

    for (int y = 0; y < modeHeight; y++) {
        for (int x = 0; x < modeWidth; x++) {
            if (vbeGetPixel(x, y) == VGA_TO_VBE(vbeTerminalBackground)) {
                vbePutPixel(x, y, VGA_TO_VBE(bg));
            }
        }
    }

    for (int y = 0; y < modeHeight; y++) {
        for (int x = 0; x < modeWidth; x++) {
            if (vbeGetPixel(x, y) == VGA_TO_VBE(vbeTerminalForeground)) {
                vbePutPixel(x, y, VGA_TO_VBE(fg));
            }
        }
    }

    vbeTerminalBackground = bg;
    vbeTerminalForeground = fg;

    vbeSwitchBuffers();
}



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
                        unsigned int c = va_arg(args, unsigned int); // We need to get the next integer.
                        char str[32] = {0}; // String buffer where the hexadecimal will go.

                        // BUGGED WITH PAGING FOR SOME REASON!

                        itoa(c, str, 16);

                        if (!print(str, strlen(str))) return -1; // Print error!

                        charsWritten++; // Increment charsWritten
                        i++; // Increment i.
                        break;
                    }


                    // %u or %U is for an unsigned integer
                    case 'U':
                    case 'u': {
                        unsigned int c = va_arg(args, int);
                        char str[32] = {0};

                        // itoa() takes care of the negative stuff
                        itoa(c, str, 10);

                        if (!print(str, strlen(str))) return -1;

                        charsWritten++;
                        i++;
                        break;
                    }
                    default:
                        terminalPutchar(format[i]); // If else place the character
                        break;
                }

                break;
            
            default:
                terminalPutchar(format[i]); // If else place the character
                break;

        }
    }
    
    if (terminalMode == 1) {
        updateTextCursor_vesa(); // Update the cursor
        vbeSwitchBuffers(); // Call after done writing to remove typewriting effect.
    }


    va_end(args);
    return charsWritten;
} 



// printf_hex(uint32_t hex) - Fixed function to get around printf's buggy %x.
void printf_hex(uint32_t hex) {
    char noZeroes = 1;
    int i;
    int32_t tmp;
    for (i = 28; i > 0; i-= 4) {
        tmp = (hex >> i) & 0xF;
        if (tmp == 0 && noZeroes != 0) continue;
        if (tmp >= 0xA) { noZeroes = 0; terminalPutchar(tmp-0xA+'a'); }
        else { noZeroes = 0; terminalPutchar(tmp+'0'); }
    }
    tmp = hex & 0xF;
    if (tmp >= 0xA) terminalPutchar(tmp-0xA + 'a');
    else terminalPutchar(tmp+'0');
}

// THESE ARE SOME MISC FUNCTIONS - MAINLY USED IN serial.c AND FUNCTIONS THAT REQUIRE A SPECIAL PRINTF (that uses a special putchar() method)


// This function is basically just printf(), except it uses put_method instead.
void printf_putchar(const char *format, void(*put_method)(char), va_list args) {
    if (!format) return; // Don't bother if there is no string

    int charsWritten = 0; // This is the value that will be returned on finish

    for (int i = 0; i < strlen(format); i++) {

        switch (format[i]) { // Take the i character of our string format and determine whether it needs special handling.

            case '%': // In the case that it's a percent sign, check what character is after it.

                switch(format[i+1]) {
                    // There are a few possible characters after this, mainly %c, %s, %d, %x, and %i
                    // %c is for characters.
                    case 'c': {
                        char c = va_arg(args, char); // We need to get the next argument that is a character.
                        
                        
                        put_method(c);

                        charsWritten++; // Increment charsWritten.
                        i++; // Increment i.
                        break;
                    }

                    // %s is for strings (or address of).
                    case 's': {
                        const char *c = va_arg(args, const char *); // Char promotes to integer, and we're trying to get the address of the string.
                        char *s = {0}; // The array where the character will go.

                        strcpy(s, (const char *)c); // Copy c to str (dest is the first parameter)
                        
                        for (int i = 0; i < strlen(s); i++) {
                            put_method(s[i]);
                        }
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
                        
                        
                        for (int i = 0; i < strlen(str); i++) 
                            put_method(str[i]);
    

                        charsWritten++; // Increment charsWritten
                        i++; // Increment i
                        break;

                    }

                    // %x or %X is for hexadecimal
                    case 'X':
                    case 'x': {
                        unsigned int c = va_arg(args, unsigned int); // We need to get the next integer (it will be converted to hexadecimal when we use itoa.) from the parameters.
                        char str[32] = {0}; // String buffer where the hexadecimal will go.

                        // itoa() will convert the integer to base-16 or hex.
                        itoa(c, str, 16); // Convert the integer to a hex string.

                        for (int i = 0; i < strlen(str); i++) 
                            put_method(str[i]);
    
                        charsWritten++; // Increment charsWritten
                        i++; // Increment i.
                        break;
                    }

                    // %u or %U is for an unsigned integer
                    case 'U':
                    case 'u': {
                        unsigned int c = va_arg(args, unsigned int);
                        char str[32] = {0};

                        // itoa() takes care of the negative stuff
                        itoa(c, str, 10);

                        for (int i = 0; i < strlen(str); i++) 
                            put_method(str[i]);
    
                        charsWritten++;
                        i++;
                        break;
                    }
                    default:
                        put_method(format[i]); // If else place the character
                        // Double check if the character was a newline or not - since serial doesn't reset the X.
                        if (format[i] == '\n') {
                            put_method('\r'); // Do a return carriage as well.
                        }
                        break;
                }

                break;
            
            default:
                put_method(format[i]);
                // Double check if the character was a newline or not - since serial doesn't reset the X.
                if (format[i] == '\n') {
                    put_method('\r'); // Do a return carriage as well.
                }
                break;

        }
    }

    va_end(args);
    return charsWritten;
}