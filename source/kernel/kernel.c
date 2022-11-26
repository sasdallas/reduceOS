// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file


// updateBottomText() - A kernel function to make handling the beginning graphics easier.
void updateBottomText(char *bottomText) {
    if (strlen(bottomText) > INT_MAX) return -1; // Overflow
    
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY));

    for (int i = 0; i < SCREEN_WIDTH - 1; i++) terminalPutcharXY(' ', vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY), i, SCREEN_HEIGHT - 1);
    terminalWriteStringXY(bottomText, 0, SCREEN_HEIGHT - 1);


    updateTerminalColor(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    return;
}


void kmain() {
    initTerminal(); // Initialize the terminal and clear the screen
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY)); // Update terminal color

    printf("reduceOS v1.0 - Development Build");
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 - Development Build")); i++) printf(" ");

    updateTerminalColor(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    printf("reduceOS is loading, please wait...\n");

    updateBottomText("Initializing GDT...");
    gdtInitialize();

    updateBottomText("Initializing IDT...");
    idtInit(0x8);
    
    

}