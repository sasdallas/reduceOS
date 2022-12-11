// =====================================================================
// isr.c - Interrupt Service Routines
// This file handles setting up the Interrupt Service Routines (ISR)
// =====================================================================

#include "include/isr.h" // Main header file

ISR interruptHandlers[256]; // A list of all interrupt handlers
 


// isrRegisterInterruptHandler(int num, ISR handler) - Registers an interrupt handler.
void isrRegisterInterruptHandler(int num, ISR handler) {
    if (num < 256) { interruptHandlers[num] = handler; }
}

// isrEndInterrupt(int num) - Ends an interrupt.
void isrEndInterrupt(int num) {
    interruptCompleted(num); // Notify HAL interrupt is completed.
}

void isrExceptionHandler(REGISTERS reg) {
    if (reg.int_no < 32) {
        panic("i86", "ISR Exception", exception_messages[reg.int_no]);
    }

    if (interruptHandlers[reg.int_no] != NULL) {
        ISR handler = interruptHandlers[reg.int_no];
        handler(&reg);
    }

}

void isrIRQHandler(REGISTERS *reg) {
    if (interruptHandlers[reg->int_no] != NULL) {
        ISR handler = interruptHandlers[reg->int_no];
        handler(reg);
    }
    interruptCompleted(reg->int_no);
}

void isrInstall() {
    for (int i = 0; i < 32; i++) {
        setVector(i, isrExceptionHandler);
        isrRegisterInterruptHandler(i, isrExceptionHandler);
    }

    for (int i = 32; i < 48; i++) {
        setVector(i, isrIRQHandler);
        isrRegisterInterruptHandler(i, isrIRQHandler);
    }  

    setVector(128, isrExceptionHandler);
    isrRegisterInterruptHandler(128, isrExceptionHandler);



    printf("Exception handlers installed.\n");
}