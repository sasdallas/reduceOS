// isr.h - header file for isr.c
// ISR - Interrupt Service Routines

#ifndef ISR_H
#define ISR_H

// Includes

#include "include/libc/stdint.h" // Integer declarations like uint8_t, int16_t, etc.
#include "include/panic.h" // Kernel panicking. Used on i86DefaultHandler.
#include "include/hal.h" // Some misc functions for interrupts.

// Definitions



// IRQ default constants
#define IRQ_BASE            0x20 // Base IRQ
#define IRQ0_TIMER          0x00 // Timer IRQ
#define IRQ1_KEYBOARD       0x01 // Keyboard IRQ
#define IRQ2_CASCADE        0x02 // Cascade IRQ
#define IRQ3_SERIAL_PORT2   0x03 // Serial 2 IRQ
#define IRQ4_SERIAL_PORT1   0x04 // Serial 1 IRQ
#define IRQ5_RESERVED       0x05 // Reserved
#define IRQ6_DISKETTE_DRIVE 0x06 // Disk drive IRQ
#define IRQ7_PARALLEL_PORT  0x07 // Parallel IRQ
#define IRQ8_CMOS_CLOCK     0x08 // CMOS IRQ
#define IRQ9_CGA            0x09 // CGA graphics IRQ
#define IRQ10_RESERVED      0x0A // Reserved
#define IRQ11_RESERVED      0x0B // Reserved
#define IRQ12_AUXILIARY     0x0C // Auxilary IRQ
#define IRQ13_FPU           0x0D // FPU IRQ
#define IRQ14_HARD_DISK     0x0E // HDD IRQ
#define IRQ15_RESERVED      0x0F // Reserved



// Typedefs

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed by pusha
    uint32_t int_no, err_code;                        // Interrupt # and error code
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by the processor automatically
} REGISTERS;

typedef void (*ISR)(REGISTERS *);


static char *exception_messages[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available (no math coprocessor)",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection",
    "Page fault",
    "Unknown interrupt (intel reserved)",
    "x87 FPU floating-point error (math fault)",
    "Alignment check",
    "Machine check",
    "SIMD floating-point exception",
    "Virtualization exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Functions

void isrRegisterInterruptHandler(int num, ISR handler); // Registers an interrupt handler
void isrEndInterrupt(int num); // Ends an interrupt.
void isrInstall(); // Installs ISR

#endif