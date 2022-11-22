// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/libc/stdbool.h" // Booleans
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions
#include "include/graphics.h" // Graphics handling
#include "include/terminal.h" // Terminal handling





void kmain() {
    initTerminal();
    printf("Hello world from the C kernel!");
    
}