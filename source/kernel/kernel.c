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

    // updateBottomText("Initializing GDT...");
    // gdtInitialize();


    updateBottomText("Initializing IDT...");
    idtInit(0x8);
    

    uint32_t vendor[32];
    
    memset(vendor, 0, sizeof(vendor));
    
    __cpuid(0x80000002, (uint32_t *)vendor+0x0, (uint32_t *)vendor+0x1, (uint32_t *)vendor+0x2, (uint32_t *)vendor+0x3);
    __cpuid(0x80000003, (uint32_t *)vendor+0x4, (uint32_t *)vendor+0x5, (uint32_t *)vendor+0x6, (uint32_t *)vendor+0x7);
    __cpuid(0x80000004, (uint32_t *)vendor+0x8, (uint32_t *)vendor+0x9, (uint32_t *)vendor+0xa, (uint32_t *)vendor+0xb);
    
    printf("CPU Vendor: %s\n", vendor);
    
    
    
    updateBottomText("Initializing PIT...");
    i86_pitInit();
    i86_pitStartCounter(100, I86_PIT_OCW_COUNTER_0, I86_PIT_OCW_MODE_SQUAREWAVEGEN);

    updateBottomText("Initializing PIC...");
    i86_picInit(0x20, 0x28);

    updateBottomText("Setting up exception handlers...");
    isrInstall();
    enableHardwareInterrupts();


    for (;;) {
        
    }
}