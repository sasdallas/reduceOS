// =============================================
// keyboard.c - reduceOS keyboard driver
// =============================================
// This file is a part of the reduceOS C kernel. Please credit me if you use it.

#include "include/keyboard.h" // Main include file


static void keyboardHandler(REGISTERS *r) {
    uint8_t scancode = inportb(0x60);
    printf("Scancode: 0x%x", scancode);
    return;
}

void keyboardInitialize() {
    isrRegisterInterruptHandler(33, keyboardHandler);
    printf("Keyboard driver initialized.\n");
}



