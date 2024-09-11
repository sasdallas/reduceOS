// ===================================================================
// terminal.c - Handles all terminal functions for graphics
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/terminal.h> // This header file contains variable declarations as not to clutter up the actual C code.
#include <kernel/vfs.h> // Only for get_cwd()
#include <kernel/font.h>
#include <kernel/video.h>
#include <libk_reduced/stdio.h>


static char *shell = "\0"; // This will be used if we're handling typing - mainly used with backspace, to prevent deleting the prompt by accident. If this variable is \0, it will not be used.


// The terminal's mode is either VGA or VESA VBE. VESA VBE is currently using PSF functions (todo: implement a system wide font API to allow for changing between different fonts)
int terminalForeground, terminalBackground;
int terminalMode; // 0 signifies VGA mode, 1 signifies VESA VBE.
int terminalEnabled = 0;

// initTerminal() - Load the terminal, setup the buffers, reset the values, etc.
void initTerminal(void) {
    terminalEnabled = 1; // Required because functions will return if terminalEnabled = 0.

    terminalX = 0, terminalY = 0;
    terminalForeground = COLOR_WHITE;
    terminalBackground = COLOR_CYAN;
    terminalBuffer = VIDEO_MEM;
    clearScreen(terminalForeground, terminalBackground);
}

// changeTerminalMode(int mode) - Changes the mode of the terminal, VGA or VBE.
void changeTerminalMode(int mode) { terminalMode = mode; }

// updateTerminalColor_gfx(uint8_t fg, uint8_t bg) - Better version of updateTerminalColor
void updateTerminalColor_gfx(uint8_t fg, uint8_t bg) {
    terminalForeground = fg;
    terminalBackground = bg;
    terminalColor = vgaColorEntry(fg, bg);

    video_setcolor(fg, bg);
}


// terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y) - Specific function to place a vgaEntry() at a specific point on the screen.
void terminalPutcharXY(unsigned char c, uint8_t color, size_t x, size_t y) {
    if (!terminalEnabled) return;
    video_putchar((char)c, x, y, color);
}

// terminalGotoXY(size_t x, size_t y) - Changes the terminal position to X and Y
void terminalGotoXY(size_t x, size_t y) { 
    terminalX = x;
    terminalY = y;
}


// This might seem bad, but it's actually not the worst.

// scrollTerminal_vesa() - I wonder what this function does.
// PRE-REVAMP
void scrollTerminal_vesa() {
    if (!terminalEnabled) return;
    if (terminalY >= modeHeight) {

        video_driver_info_t *inf = video_getInfo();
        uint32_t *fb = (uint32_t*)inf->videoBuffer;

        uint32_t sw = video_getScreenWidth();
        uint32_t sh = video_getScreenHeight();

        // Reading directly from vmem is very slow, but reading from our secondary buffer is not
        for (uint32_t y = video_getFontHeight(); y < sh; y++) {
            for (uint32_t x = 0; x < sw; x++) {
                vbePutPixel(x, y - video_getFontHeight(), *(fb + (y*1024 + x))); // Copy the pixels one line below to the current line.
            }    
        }

        // Blank the bottom line.
        for (uint32_t y = sh - video_getFontHeight(); y < sh; y++) {
            for (uint32_t x = 0; x < video_getScreenWidth(); x++) {
                vbePutPixel(x, y, VGA_TO_VBE(terminalBackground));
            }
        } 


        // Update terminalY to the new position after scrolling
        terminalY = modeHeight - video_getFontHeight();

        kfree(inf);
    }
}

// scrollTerminal() - Scrolls the terminal
void scrollTerminal() {
    if (!terminalEnabled) return;
    
    // REMAINING FROM PRE-VIDEO REVAMP
    if (terminalMode == 1) {
        scrollTerminal_vesa();
        return;
    }

    uint16_t blank = 0x20 | (terminalColor << 8);

    if (terminalY >= SCREEN_HEIGHT) {
        uint32_t i;
        for (i = 0*SCREEN_WIDTH; i < (SCREEN_HEIGHT-1) * SCREEN_WIDTH; i++) {
            terminalBuffer[i] = terminalBuffer[i+SCREEN_WIDTH];
        }

        for (uint32_t i = (SCREEN_HEIGHT - 1) * SCREEN_WIDTH; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) {
            terminalBuffer[i] = blank;
        }

        terminalY = SCREEN_HEIGHT - 1;
    }
}

// clearScreen(uint8_t fg, uint8_t bg) - Clears the screen 
void clearScreen(uint8_t fg, uint8_t bg) {
    if (!terminalEnabled) return;
    video_clearScreen(fg, bg);

    terminalX = 0; // Reset X & Y values
    terminalY = 0; 
    return;
}


// updateTextCursor_vesa() - Does the same thing but in VESA (aka much more complex) - event in PIT IRQ
// PRE-REVAMP

static bool cursorEnabled = true;
static int blinkTime = 0; // Time since last blink (~500ms)
static int blinkedLast = 0; // Did it blink last time? Do we need to clear/draw it?
void updateTextCursor_vesa() {
    if (!terminalEnabled) return;
    if (!cursorEnabled) return;

    // This function is called by pitTicks on a reasonable level
    if (blinkTime > 500) {
        if (blinkedLast) {
            for (uint32_t x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
                vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(terminalBackground));
            }
            blinkedLast = 0;
        } else {
            for (uint32_t x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
                vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(terminalForeground));
            }
            blinkedLast = 1;
        }

        vbeSwitchBuffers();
        blinkTime = 0;
    } else {
        blinkTime++;
    }
}





// If a newline is typed, remove the text cursor if it existed.
static void clearTextCursor_vesa() {
    if (!terminalEnabled) return;
    if (!cursorEnabled) return;

    if (blinkedLast) {
        for (uint32_t x = terminalX; x < terminalX + psfGetFontWidth(); x++) {
            vbePutPixel(x, terminalY + psfGetFontHeight() - 2, VGA_TO_VBE(terminalBackground));
        }
    }
}

void setCursorEnabled(bool enabled) { cursorEnabled = enabled; }

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
    if (!terminalEnabled) return;


    // Checking if c is a special character or not.
    switch (c) {
        case '\n':
            // Newline
            if (video_canHasGraphics()) clearTextCursor_vesa();
            terminalY += video_getFontHeight();
            terminalX = 0;

            // Some drivers (e.g headless driver) necessitate being informed about a newline
            // We can write a blank character to these drivers with our new Y.
            if (!video_canHasGraphics()) video_putchar('\0', terminalX, terminalY, (video_canHasGraphics() ? terminalForeground : terminalColor));
            break;
        
        case '\b':
            // Backspace
            if (video_canHasGraphics()) clearTextCursor_vesa();
            terminalBackspace();
            break;
        
        case '\0':
            // Null character
            if (video_canHasGraphics()) clearTextCursor_vesa();
            break;

        case '\t':
            // Tab
            for (int i = 0; i < 4; i++) terminalPutchar(' ');
            break;
        
        case '\r':
            // Return carriage
            terminalX = 0;
            break;
        
        default:
            // Normal character
            video_putchar(c, terminalX, terminalY, (video_canHasGraphics() ? terminalForeground : terminalColor));
            terminalX = terminalX + video_getFontWidth();
            break;
    }

    // Did the X get to screen width?
    if (terminalX == video_getScreenWidth()) {
        terminalY = terminalY + video_getFontHeight();
        terminalX = 0;
    }
    
    // Perform the scrolling stuff for Y
    scrollTerminal();

    // Update text mode cursor
    video_cursor(terminalX, terminalY);
}


// terminalWrite(const char *data, size_t size) - Writes a certain amount of data to the terminal.
void terminalWrite(const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminalPutchar(data[i]); 
    }

    // printf() will handle the terminalPutchar
}


// terminalWriteString() - The exact same as terminal write, but shorter with no len option (we use strlen). Not recommended for use, use printf (further down in the file)!
void terminalWriteString(const char *data) { terminalWrite(data, strlen((char*)data)); }

// terminalBackspace() - Removes the last character outputted.
void terminalBackspace() {
    if (!terminalEnabled) return;
    if (terminalX == 0) return; // terminalX being 0 would cause a lot of problems.
    if (terminalX <= (uint32_t)(strlen(shell) * video_getFontWidth()) && shell) return; // Cannot overwrite the shell.
    
    
    // First, go back one character.
    terminalGotoXY(terminalX - video_getFontWidth(), terminalY);

    // Then, write a space where the character is.
    terminalPutchar(' ');

    // Finally, set terminalX to one below current.
    terminalGotoXY(terminalX - video_getFontWidth(), terminalY);
}


// updateBottomText() - A kernel function to make handling the beginning graphics easier.
void updateBottomText(char *bottomText) {
    return; // Deprecated.
}

// enableShell() - Enables a boundary that cannot be overwritten.
void enableShell(char *shellToUse) { shell = shellToUse; }

// updateShell() - Updates the shell with the CWD.
void updateShell() {
    kfree(shell);
    shell = kmalloc(strlen("reduceOS > ") + strlen(get_cwd()));
    strcpy(shell, "reduceOS ");
    strcpy(shell + strlen("reduceOS "), get_cwd());
    strcpy(shell + strlen("reduceOS ") + strlen(get_cwd()), "> ");
}

// getShell() - Returns the current shell
char *getShell() { return shell; }

// instantUpdateTerminalColor(uint8_t fg, uint8_t bg) - Instantly update the terminal color (VESA only - MOVING REQUIRED)
void instantUpdateTerminalColor(uint8_t fg, uint8_t bg) {
    if (terminalMode != 1) return; // VESA only.

    // NOTE: Function breaks if it tries to update a section where fg and bg are the same.
    // The way we do this is to iterate over the entire thing two times - once to update bg, once to update fg.

    for (uint32_t y = 0; y < modeHeight; y++) {
        for (uint32_t x = 0; x < modeWidth; x++) {
            if (vbeGetPixel(x, y) == VGA_TO_VBE(terminalBackground)) {
                vbePutPixel(x, y, VGA_TO_VBE(bg));
            }
        }
    }

    for (uint32_t y = 0; y < modeHeight; y++) {
        for (uint32_t x = 0; x < modeWidth; x++) {
            if (vbeGetPixel(x, y) == VGA_TO_VBE(terminalForeground)) {
                vbePutPixel(x, y, VGA_TO_VBE(fg));
            }
        }
    }

    terminalBackground = bg;
    terminalForeground = fg;

    vbeSwitchBuffers();
}

static bool updateScreen = true;

// terminalSetUpdateScreen(bool state) - Toggles whether the screen should update when terminalUpdateScreen() is called. Useful for long printfs
void terminalSetUpdateScreen(bool state) { updateScreen = state; }

// terminalUpdateScreen() - Update the buffers on the screen
void terminalUpdateScreen() {
    if (terminalEnabled && updateScreen) video_update_screen();
}

// terminalUpdateTopBar(char *text) - Update the top bar on the screen with exact text
void terminalUpdateTopBar(char *text) {
    if (!terminalEnabled) return;

    int oldfg = terminalForeground;
    int oldbg = terminalBackground;

    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color
    
    terminalSetUpdateScreen(false);

    printf("%s", text);
    if (terminalMode == 0) {
        for (uint32_t i = 0; i < (SCREEN_WIDTH - strlen(text)); i++) printf(" "); 
    } else {
        for (uint32_t i = 0; i < ((modeWidth - ((strlen(text)) * psfGetFontWidth()))); i += psfGetFontWidth()) printf(" ");
    }
    
    terminalSetUpdateScreen(true);
    terminalUpdateScreen();

    updateTerminalColor_gfx(oldfg, oldbg);
}


// terminalUpdateTopBarKernel(char *text) - Prints out kernel version and codename too 
void terminalUpdateTopBarKernel(char *text) {
    char *buffer = kmalloc(strlen(text) + strlen(VERSION) + strlen(CODENAME));
    sprintf(buffer, "reduceOS v%s %s - %s", VERSION, CODENAME, text);
    terminalUpdateTopBar(buffer);
}
