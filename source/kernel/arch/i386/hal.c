/**
 * @file source/kernel/arch/i386/hal.c
 * @brief The reduceOS kernel hardware abstraction layer (i386 architecture)
 * 
 * This file handles call translation from the main kernel logic to bare hardware.
 * An implementation is required for a new architecture.
 * The kernel will call hal_init() to initialize the HAL.
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/hal.h> // Main header file
#include <kernel/arch/i386/processor.h>



/**
 * @brief Temporary hal shenanigans. Ignore!
 */
ISR hal_cpuidIsAvailable(registers_t *reg) {
    // hit it
    panic_prepare();
    serialPrintf("*** The system does not support the CPUID instruction.\n");
    serialPrintf("*** reduceOS requires the CPUID instruction to operate correctly.\n");
    serialPrintf("\nThe video drivers have not been initialized. The system will now reboot.\n");
    hal_reboot();
    __builtin_unreachable();
}

/**
 * @brief Initialize the hardware abstraction layer.
 */

void hal_init() {
    serialPrintf("[i386] Hardware abstraction layer (HAL) starting up...\n");
    
    // Initialize the CPU
    processor_init();

    // Make sure the CPU is compatible
    // CPUID check. Remap an interrupt handler for the Opcode Error
    isrRegisterInterruptHandler(6, (ISR)hal_cpuidIsAvailable);

    // Tense!
    uint32_t discard;
    __cpuid(0, &discard, &discard, &discard, &discard);

    // All good! Unregister that handler.
    isrUnregisterInterruptHandler(6);

    // Collect processor data (also initializes FPU)
    processor_collect_data();
    
    serialPrintf("[i386] HAL initialized successfully.\n");
}

/**
 * @brief Reboot the system
 */
void hal_reboot() {
    for (;;);
}


/** INTERRUPT FUNCTIONS **/

// void hal_interruptCompleted(uint32_t intNo) - Notifies HAL interrupt is done.
void hal_interruptCompleted(uint32_t intNo) {
    // Do we need to send EOI to second PIC?
    if (intNo >= 40) outportb(0xA0, 0x20);
    
    // Send EOI to master
    outportb(0x20, 0x20);
}


// void hal_setInterruptVector(int intNo, uint32_t vect) - Sets a new interrupt vector.
void hal_setInterruptVector(int intNo, uint32_t vect) {
    idtInstallIR(intNo,  0x8E, 0x08, (uint32_t)vect);
}

// void setVector_flags(int intNo, uint32_t vect, int flags) - Sets a new interrupt vector using flags.
void hal_setInterruptVectorFlags(int intNo, uint32_t vect, int flags) {
    idtInstallIR(intNo, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32 | flags, 0x8, vect);
}

// void enableHardwareInterrupts() - Enable hardware interrupts
void hal_enableHardwareInterrupts() {
    asm ("sti");
}


// void disableHardwareInterrupts() - Disable hardware interrupts
void hal_disableHardwareInterrupts() {
    asm ("cli");
}

/** I/O PORT MANIPULATION **/

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

// inportw(unsigned short port) - Reads word from device
uint16_t inportw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// outportw(unsigned short port, unsigned short data) - Writes word to device
void outportw(uint16_t port, uint16_t data) {
    asm volatile ("outw %1, %0" :: "Nd"(port), "a"(data));
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


/** MISCALLANEOUS HAL FUNCTIONS **/

// void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) - Returns an assembly cpuid instruction's results.
void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
	asm volatile("cpuid"
				: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
				: "0"(type)); // Add type to eax
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


// unsigned short hal_getBIOSArea(unsigned short offset) - Returns the value at the offset in the BDA
unsigned short hal_getBIOSArea(unsigned short offset) {
    if (HAL_BDA_START + offset > HAL_BDA_END) return 0x0;
    const uint16_t *bda_area = (const uint16_t*)(HAL_BDA_START + offset);
    return *bda_area;
}
