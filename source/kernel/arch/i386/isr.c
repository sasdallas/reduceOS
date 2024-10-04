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
    hal_interruptCompleted(interruptNumber);
    interruptToBeAcknowledged = -1;
}

// isrRegisterInterruptHandler(int num, ISR handler) - Registers an interrupt handler.
void isrRegisterInterruptHandler(int num, ISR handler) {
    if (num < 256) { interruptHandlers[num] = handler; }
}

// isrUnregisterInterruptHandler(int num) - Unregister an interrupt handler
void isrUnregisterInterruptHandler(int num) {
    if (num < 256) { interruptHandlers[num] = NULL; }
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
    hal_setInterruptVector(0, (uint32_t)isr0);
    hal_setInterruptVector(1, (uint32_t)isr1);
    hal_setInterruptVector(2, (uint32_t)isr2);
    hal_setInterruptVector(3, (uint32_t)isr3);
    hal_setInterruptVector(4, (uint32_t)isr4);
    hal_setInterruptVector(5, (uint32_t)isr5);
    hal_setInterruptVector(6, (uint32_t)isr6);
    hal_setInterruptVector(7, (uint32_t)isr7);
    hal_setInterruptVector(8, (uint32_t)isr8);
    hal_setInterruptVector(9, (uint32_t)isr9);
    hal_setInterruptVector(10, (uint32_t)isr10);
    hal_setInterruptVector(11, (uint32_t)isr11);
    hal_setInterruptVector(12, (uint32_t)isr12);
    hal_setInterruptVector(13, (uint32_t)isr13);
    hal_setInterruptVector(14, (uint32_t)isr14);
    hal_setInterruptVector(15, (uint32_t)isr15);
    hal_setInterruptVector(16, (uint32_t)isr16);
    hal_setInterruptVector(17, (uint32_t)isr17);
    hal_setInterruptVector(18, (uint32_t)isr18);
    hal_setInterruptVector(19, (uint32_t)isr19);
    hal_setInterruptVector(20, (uint32_t)isr20);
    hal_setInterruptVector(21, (uint32_t)isr21);
    hal_setInterruptVector(22, (uint32_t)isr22);
    hal_setInterruptVector(23, (uint32_t)isr23);
    hal_setInterruptVector(24, (uint32_t)isr24);
    hal_setInterruptVector(25, (uint32_t)isr25);
    hal_setInterruptVector(26, (uint32_t)isr26);
    hal_setInterruptVector(27, (uint32_t)isr27);
    hal_setInterruptVector(28, (uint32_t)isr28);
    hal_setInterruptVector(29, (uint32_t)isr29);
    hal_setInterruptVector(30, (uint32_t)isr30);
    hal_setInterruptVector(31, (uint32_t)isr31);

    // Register all IRQs
    hal_setInterruptVector(32, (uint32_t)irq_0);
    hal_setInterruptVector(33, (uint32_t)irq_1);
    hal_setInterruptVector(34, (uint32_t)irq_2);
    hal_setInterruptVector(35, (uint32_t)irq_3);
    hal_setInterruptVector(36, (uint32_t)irq_4);
    hal_setInterruptVector(37, (uint32_t)irq_5);
    hal_setInterruptVector(38, (uint32_t)irq_6);
    hal_setInterruptVector(39, (uint32_t)irq_7);
    hal_setInterruptVector(40, (uint32_t)irq_8);
    hal_setInterruptVector(41, (uint32_t)irq_9);
    hal_setInterruptVector(42, (uint32_t)irq_10);
    hal_setInterruptVector(43, (uint32_t)irq_11);
    hal_setInterruptVector(44, (uint32_t)irq_12);
    hal_setInterruptVector(45, (uint32_t)irq_13);
    hal_setInterruptVector(46, (uint32_t)irq_14);
    hal_setInterruptVector(47, (uint32_t)irq_15);

    // Register exception 128 (SYSCALLS)
    hal_setInterruptVectorFlags(128, (uint32_t)isr128, I86_IDT_DESC_RING3);
}
