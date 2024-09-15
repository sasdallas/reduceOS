// ==================================================
// isr.c - Interrupt Service Routines initializer
// ==================================================

#include <kernel/clock.h>
#include <kernel/isr.h> // Main header file
#include <kernel/process.h>
#include <kernel/signal.h>

ISR interruptHandlers[256];         // A list of all interrupt handlers
int interruptToBeAcknowledged = -1; // When an interrupt is fired, the interrupt number will be logged here.

#pragma GCC diagnostic ignored                                                                                         \
    "-Wint-conversion" // Lots of warnings about setvector() not taking an unsigned int. Ignore them all.

// isrAcknowledge(uint32_t interruptNumber) - Acknowledge the current interrupt
void isrAcknowledge(uint32_t interruptNumber) {
    interruptCompleted(interruptNumber);
    interruptToBeAcknowledged = -1;
}

// isrRegisterInterruptHandler(int num, ISR handler) - Registers an interrupt handler.
void isrRegisterInterruptHandler(int num, ISR handler) {
    if (num < 256) { interruptHandlers[num] = handler; }
}

void isrExceptionHandler(registers_t* reg) {
    bool handled = false;
    if (interruptHandlers[reg->int_no] != NULL) {
        ISR handler = interruptHandlers[reg->int_no];
        handler(reg);
        handled = true;
    }

    // Userspace exceptions are not our fault
    if (reg->cs != 0x08 && !handled) {
        serialPrintf("[i386] WARNING: A fault was detected in the current process %s. %s - exception number %i\n",
                     currentProcess->name, exception_messages[reg->int_no], reg->int_no);

        send_signal(currentProcess->id, SIGABRT, 1);
        return;
    }

    if (reg->int_no < 32 && !handled) { panicReg("i86", "ISR Exception", exception_messages[reg->int_no], reg); }

    return;
}

void isrIRQHandler(registers_t* reg) {
    interruptToBeAcknowledged = reg->int_no;

    // Some debug code I left in in case anyone is modifying reduceOS.
    // if (true) serialPrintf("isrIRQHandler received IRQ. IRQ: %i. Valid handler present: %s. INT NO: %i. Valid handler present (for INT_NO): %s\n", reg->err_code, (interruptHandlers[reg->err_code]) ? "YES" : "NO", reg->int_no, (interruptHandlers[reg->int_no]) ? "YES" : "NO");

    // First, check if the call is from userspace.
    int from_userspace = reg->cs != 0x08;

    if (from_userspace && currentProcess) { currentProcess->time_switch = clock_getTimer(); }

    if (interruptHandlers[reg->int_no] != NULL) {
        // We now know there is a valid handler present. Execute it.
        ISR handler = interruptHandlers[reg->int_no];
        handler(reg);
    }

    // If the call is from userspace, we need to update the process times on eixt
    if (from_userspace && currentProcess) {
        process_check_signals(reg);
        updateProcessTimesOnExit();
    }

    // Send EOI to PIC if the handler didn't already
    if (interruptToBeAcknowledged != -1) { isrAcknowledge(reg->int_no); }
}

void isrInstall() {
    // First, install the proper ISR exception handlers into the system.
    setVector(0, (uint32_t)isr0);
    setVector(1, (uint32_t)isr1);
    setVector(2, (uint32_t)isr2);
    setVector(3, (uint32_t)isr3);
    setVector(4, (uint32_t)isr4);
    setVector(5, (uint32_t)isr5);
    setVector(6, (uint32_t)isr6);
    setVector(7, (uint32_t)isr7);
    setVector(8, (uint32_t)isr8);
    setVector(9, (uint32_t)isr9);
    setVector(10, (uint32_t)isr10);
    setVector(11, (uint32_t)isr11);
    setVector(12, (uint32_t)isr12);
    setVector(13, (uint32_t)isr13);
    setVector(14, (uint32_t)isr14);
    setVector(15, (uint32_t)isr15);
    setVector(16, (uint32_t)isr16);
    setVector(17, (uint32_t)isr17);
    setVector(18, (uint32_t)isr18);
    setVector(19, (uint32_t)isr19);
    setVector(20, (uint32_t)isr20);
    setVector(21, (uint32_t)isr21);
    setVector(22, (uint32_t)isr22);
    setVector(23, (uint32_t)isr23);
    setVector(24, (uint32_t)isr24);
    setVector(25, (uint32_t)isr25);
    setVector(26, (uint32_t)isr26);
    setVector(27, (uint32_t)isr27);
    setVector(28, (uint32_t)isr28);
    setVector(29, (uint32_t)isr29);
    setVector(30, (uint32_t)isr30);
    setVector(31, (uint32_t)isr31);

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

    // Register exception 128 (SYSCALLS)
    setVector_flags(128, (uint32_t)isr128, I86_IDT_DESC_RING3);
}
