// =====================================================================
// isr.c - Interrupt Service Routines
// This file handles setting up the Interrupt Service Routines (ISR)
// =====================================================================

#include "include/isr.h" // Main header file

ISR interruptHandlers[256]; // A list of all interrupt handlers
 
#pragma GCC diagnostic ignored "-Wint-conversion" // Lots of warnings about setvector() not taking an unsigned int. Ignore them all.

// isrRegisterInterruptHandler(int num, ISR handler) - Registers an interrupt handler.
void isrRegisterInterruptHandler(int num, ISR handler) {
    if (num < 256) { interruptHandlers[num] = handler; }
}

// isrEndInterrupt(int num) - Ends an interrupt.
void isrEndInterrupt(int num) {
    interruptCompleted((uint32_t) num); // Notify HAL interrupt is completed.
}

void isrExceptionHandler(REGISTERS *reg) {
    if (reg->err_code < 32) {
        panicReg("i86", "ISR Exception", exception_messages[reg->err_code], reg);
    }

    if (interruptHandlers[reg->err_code] != NULL) {
        ISR handler = interruptHandlers[reg->err_code];
        handler(reg);
    }

}

void isrIRQHandler(REGISTERS *reg) {
    // Some debug code I left in in case anyone is modifying reduceOS.
    // printf("isrIRQHandler received IRQ. IRQ: %i. Valid handler present: %s.\nINT NO: %i. Valid handler present (for INT_NO): %s", reg->err_code, (interruptHandlers[reg->err_code]) ? "YES" : "NO", reg->int_no, (interruptHandlers[reg->int_no]) ? "YES" : "NO");
    
    if (interruptHandlers[reg->int_no] != NULL) {
        // We now know there is a valid handler present. Execute it.
        ISR handler = interruptHandlers[reg->int_no];
        handler(reg);
    }
    
    // Send EOI to PIC (this function is present in hal.h)
    
    interruptCompleted(reg->err_code);
    
}

void isrInstall() {
    // First, install the proper ISR exception handlers into the system.
    setVector(0, (uint32_t) isr0);
    setVector(1, (uint32_t) isr1);
    setVector(2, (uint32_t) isr2);
    setVector(3, (uint32_t) isr3);
    setVector(4, (uint32_t) isr4);
    setVector(5, (uint32_t) isr5);
    setVector(6, (uint32_t) isr6);
    setVector(7, (uint32_t) isr7);
    setVector(8, (uint32_t) isr8);
    setVector(9, (uint32_t) isr9);
    setVector(10, (uint32_t) isr10);
    setVector(11, (uint32_t) isr11);
    setVector(12, (uint32_t) isr12);
    setVector(13, (uint32_t) isr13);
    setVector(14, (uint32_t) isr14);
    setVector(15, (uint32_t) isr15);
    setVector(16, (uint32_t) isr16);
    setVector(17, (uint32_t) isr17);
    setVector(18, (uint32_t) isr18);
    setVector(19, (uint32_t) isr19);
    setVector(20, (uint32_t) isr20);
    setVector(21, (uint32_t) isr21);
    setVector(22, (uint32_t) isr22);
    setVector(23, (uint32_t) isr23);
    setVector(24, (uint32_t) isr24);
    setVector(25, (uint32_t) isr25);
    setVector(26, (uint32_t) isr26);
    setVector(27, (uint32_t) isr27);
    setVector(28, (uint32_t) isr28);
    setVector(29, (uint32_t) isr29);
    setVector(30, (uint32_t) isr30);
    setVector(31, (uint32_t) isr31);

    // Initialize PIC
    // i86_picInit(0x20, 0x28);
    
    // Register all IRQs
    setVector(32, (uint32_t)irq_0);
    setVector(33, (uint32_t)irq_1);
    setVector(34, (uint32_t)irq_2);
    setVector(35, (uint32_t)irq_3);
    setVector(36, (uint32_t)irq_4);
    setVector(37, (uint32_t)irq_5);
    setVector(38, (uint32_t)irq_6);
    setVector(39, (uint32_t)irq_7);
    setVector(40, (uint32_t)irq_8);
    setVector(41, (uint32_t)irq_9);
    setVector(42, (uint32_t)irq_10);
    setVector(43, (uint32_t)irq_11);
    setVector(44, (uint32_t)irq_12);
    setVector(45, (uint32_t)irq_13);
    setVector(46, (uint32_t)irq_14);
    setVector(47, (uint32_t)irq_15);
    
    // Register exception 128
    setVector(128, (uint32_t)isr128);

    // Done!
    printf("Exception handlers installed.\n");
}