// =====================================================================
// hal.c - Hardware Abstraction Layer
// This file handles functions of the Hardware Abstraction Layer (HAL)
// =====================================================================

#include "include/hal.h" // Main header file


// void interruptCompleted(uint32_t intNo) - Notifies HAL interrupt is done.
void interruptCompleted(uint32_t intNo) {
    // Ensure its a valid hardware IRQ
    if (intNo > 16) return;

    // Do we need to send EOI to second PIC?
    if (intNo >= 8) i86_picSendCommand(I86_PIC_OCW2_MASK_EOI, 1);

    // Always send EOI to first PIC.
    i86_picSendCommand(I86_PIC_OCW2_MASK_EOI, 0);
}


// void setVector(int intNo, far vect) - Sets a new interrupt vector.
void setVector(int intNo, void (far vect) ( ) ) {
    idtInstallIR(intNo, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x8, vect);
}

// void enableHardwareInterrupts() - Enable hardware interrupts
void enableHardwareInterrupts() {
    asm volatile ("sti");
}


// void disableHardwareInterrupts() - Disable hardware interrupts
void disableHardwareInterrupts() {
    asm volatile ("cli");
}

// uint8_t inportb(uint16_t port) - Read data from device through port mapped IO
uint8_t inportb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// void outportb(unsigned short port, unsigned char value) - write byte to device through port mapped IO
void outportb(uint16_t port, uint8_t value) {
    asm volatile("outb %1, %0" :: "dN"(port), "a"(value));
}