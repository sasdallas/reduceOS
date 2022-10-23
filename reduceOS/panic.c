#include "panic.h"
#include "console.h"


void kernel_panic(const char* pmreason, ...) {
    clearConsole(COLOR_WHITE, COLOR_BLACK);
    printf("ERROR! reduceOS encountered a fatal error and will now be shutting down.");
    printf("*** STOP: %s", pmreason);
    
    asm volatile ("hlt");
}