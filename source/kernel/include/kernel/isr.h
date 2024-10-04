// isr.h - header file for isr.c
// ISR - Interrupt Service Routines

#ifndef ISR_H
#define ISR_H

// Includes

#include <libk_reduced/stdint.h> // Integer declarations like uint8_t, int16_t, etc.
#include <kernel/hal.h> // Some misc functions for interrupts.


#include <kernel/panic.h> // Kernel panicking. Used on i86DefaultHandler.
#include <kernel/regs.h> // registers_t typedef

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

typedef void (*ISR)(registers_t  *);

// Variable definitions

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



// External declarations
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr128();

extern void irq_0();
extern void irq_1();
extern void irq_2();
extern void irq_3();
extern void irq_4();
extern void irq_5();
extern void irq_6();
extern void irq_7();
extern void irq_8();
extern void irq_9();
extern void irq_10();
extern void irq_11();
extern void irq_12();
extern void irq_13();
extern void irq_14();
extern void irq_15();

// Functions

void isrRegisterInterruptHandler(int num, ISR handler);         // Registers an interrupt handler
void isrUnregisterInterruptHandler(int num);                    // Unregister an interrupt handler
void isrEndInterrupt(int num);                                  // Ends an interrupt.
void isrInstall();                                              // Installs ISR
void isrAcknowledge(uint32_t interruptNumber);                  // Acknowledge an interrupt
#endif
