// =====================================================================
// panic.c - Kernel panic
// This file handles the kernel panicking and exiting
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.


#include "include/panic.h" // Main panic include file

void *panic(char *caller, char *code, char *reason) {
    clearScreen(terminalColor);
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY)); // Update terminal color

    printf("reduceOS v1.0 (Development Build) - Kernel Panic");
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 (Development Build) - Kernel Panic")); i++) printf(" ");

    updateBottomText("A fatal error occurred!");

    printf("reduceOS encountered a fatal error and needs to shutdown.\n");
    printf("The error cause will be printed below. If you start an issue on GitHub, please include the following text.\n");
    printf("Apologies for any inconveniences caused by this error.\n");
    printf("\n");
    printf("The error encountered was:\n");
    printf("*** [%s] %s: %s \n", caller, code, reason);
    printf("\nStack dump:\n");
    
    printf("N/A\n");

    asm volatile ("hlt");
}