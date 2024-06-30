// =====================================================================
// hal.c - Hardware Abstraction Layer
// This file handles functions of the Hardware Abstraction Layer (HAL)
// =====================================================================

#include "include/hal.h" // Main header file


// Functions

// void interruptCompleted(uint32_t intNo) - Notifies HAL interrupt is done.
void interruptCompleted(uint32_t intNo) {
    outportb(0x20, 0x20);

    // Do we need to send EOI to second PIC?
    if (intNo >= 40) outportb(0xA0, 0x20);
}


// void setVector(int intNo, uint32_t vect) - Sets a new interrupt vector.
void setVector(int intNo, uint32_t vect) {
    idtInstallIR(intNo,  0x8E, 0x08, (uint32_t)vect);
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

// void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) - Returns an assembly cpuid instruction's results.
void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
	asm volatile("cpuid"
				: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
				: "0"(type)); // Add type to eax
}

// uint32_t inportl(uint16_t port) - Reads data from device via port mapped IO
uint32_t inportl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

// void outportl(uint16_t port, uint8_t value) - Write byte to device via port mapped IO
void outportl(uint16_t port, uint32_t value) {
    asm volatile ("outl %1, %0" :: "dN"(port), "a"(value));
}


// size_t msb(size_t i) - Returns the most significant bit.
size_t msb(size_t i)
{
	size_t ret;

	if (!i)
		return (sizeof(size_t)*8);
	asm volatile ("bsr %1, %0" : "=r"(ret) : "r"(i) : "cc");

	return ret;
}

