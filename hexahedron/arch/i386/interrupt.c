/**
 * @file hexahedron/arch/i386/interrupt.c
 * @brief Interrupt handler & register
 * 
 * This file handles setting up IDT/GDT/TSS/etc. and registering handlers.
 * @see interrupt.h for the external handlers
 * @see irq.S for the Assembly-based parts of this code
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/interrupt.h>
#include <kernel/arch/i386/registers.h>
#include <kernel/arch/i386/smp.h>
#include <kernel/arch/i386/arch.h>
#include <kernel/hal.h>

#include <kernel/debug.h>
#include <kernel/panic.h>

#include <stdint.h>
#include <string.h>
#include <errno.h>

/* irq.S functions (note: all handlers defined in interrupt.h) */
extern void hal_installGDT();

/* IDT */
i386_interrupt_descriptor_t hal_idt_table[I86_MAX_INTERRUPTS] = { 0 };

/* Interrupt handler table - TODO: More than one handler per interrupt? */
interrupt_handler_t hal_handler_table[I86_MAX_INTERRUPTS] = { 0 };

/* Exception handler table - TODO: More than one handlers per exception? */
exception_handler_t hal_exception_handler_table[I86_MAX_EXCEPTIONS] = { 0 };

/* Context table (makes drivers have a better time with interrupts) */
void *hal_interrupt_context_table[I86_MAX_INTERRUPTS] = { 0 };

/* String table for exceptions */
const char *hal_exception_table[I86_MAX_EXCEPTIONS] = {
    "division error",
    "debug trap",
    "NMI exception",
    "breakpoint trap",
    "overflow trap",
    "bound range exceeded",
    "invalid opcode",
    "device not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack-segment fault",
    "general protection fault",
    "page fault",
    "reserved",
    "FPU exception",
    "alignment check",
    "machine check",
    "SIMD floating-point exception",
    "virtualization exception",
    "control protection exception",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "hypervisor injection exception",
    "VMM communication exception",
    "security exception"
};



/**
 * @brief Handle ending an interrupt
 */
void hal_endInterrupt(uint32_t interrupt_number) {
    if (interrupt_number > 8) outportb(I86_PIC2_COMMAND, I86_PIC_EOI);
    outportb(I86_PIC1_COMMAND, I86_PIC_EOI);
}

/**
 * @brief Common exception handler
 */
void hal_exceptionHandler(uintptr_t exception_index, registers_t *regs, extended_registers_t *regs_extended) {
    // Call the exception handler
    if (hal_exception_handler_table[exception_index] != NULL) {
        exception_handler_t handler = (hal_exception_handler_table[exception_index]);
        int return_value = handler(exception_index, regs, regs_extended);

        if (return_value != 0) {
            kernel_panic(IRQ_HANDLER_FAILED, "hal");
            __builtin_unreachable();
        }

        // Now we're finished so return
        return;
    }

    // NMIs are fired as of now only for a core shutdown. If we receive one, just halt.
    if (exception_index == 2) {
        smp_acknowledgeCoreShutdown();
        for (;;);
    }


    // Looks like no one caught this exception.
    kernel_panic_prepare(CPU_EXCEPTION_UNHANDLED);

    if (exception_index == 14) {
        uintptr_t page_fault_addr = 0x0;
        asm volatile ("movl %%cr2, %0" : "=a"(page_fault_addr));
        dprintf(NOHEADER, "*** ISR detected exception: Page fault at address 0x%08X\n\n", page_fault_addr);
        printf("*** Page fault at address 0x%08X\n", page_fault_addr);
    } else if (exception_index < I86_MAX_EXCEPTIONS) {
        dprintf(NOHEADER, "*** ISR detected exception %i - %s\n\n", exception_index, hal_exception_table[exception_index]);
        printf("*** ISR detected exception %i - %s\n", exception_index, hal_exception_table[exception_index]);
    } else {
        dprintf(NOHEADER, "*** ISR detected exception %i - UNKNOWN TYPE\n\n", exception_index);
        printf("*** ISR detected unknown exception: %i\n", exception_index);
    }
    
    

    dprintf(NOHEADER, "\033[1;31mFAULT REGISTERS:\n\033[0;31m");

    dprintf(NOHEADER, "EAX %08X EBX %08X ECX %08X EDX %08X\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    dprintf(NOHEADER, "EDI %08X ESI %08X EBP %08X ESP %08X\n", regs->edi, regs->esi, regs->ebp, regs->esp);
    dprintf(NOHEADER, "ERR %08X EIP %08X\n\n", regs->err_code, regs->eip);
    dprintf(NOHEADER, "CS %04X DS %04X ES %04X GS %04X\n", regs->cs, regs->ds, regs->es, regs->gs);
    dprintf(NOHEADER, "GDTR %08X %04X\nIDTR %08X %04X\n", regs_extended->gdtr.base, regs_extended->gdtr.limit, regs_extended->idtr.base, regs_extended->idtr.limit);

    // !!!: not conforming (should call kernel_panic_finalize) but whatever
    // We want to do our own traceback.
    arch_panic_traceback(10, regs);

    // Display message
    dprintf(NOHEADER, COLOR_CODE_RED "\nThe kernel will now permanently halt. Connect a debugger for more information.\n");

    // Disable interrupts & halt
    asm volatile ("cli\nhlt");
    for (;;);

    // kernel_panic_finalize();
}

/**
 * @brief Common interrupt handler
 */
void hal_interruptHandler(uintptr_t exception_index, uintptr_t int_number, registers_t *regs, extended_registers_t *regs_extended) {
    // Call any handler registered
    if (hal_handler_table[int_number] != NULL) {
        int return_value = 1;

        // Context specified?
        if (hal_interrupt_context_table[int_number] != NULL) {
            interrupt_handler_context_t handler = (interrupt_handler_context_t)(hal_handler_table[int_number]);
            return_value = handler(hal_interrupt_context_table[int_number]);
        } else {
            interrupt_handler_t handler = (hal_handler_table[int_number]);
            return_value = handler(exception_index, int_number, regs, regs_extended);
        }

        if (return_value != 0) {
            kernel_panic(IRQ_HANDLER_FAILED, "hal");
            __builtin_unreachable();
        }
    }

    hal_endInterrupt(int_number);
}

/**
 * @brief Register an interrupt handler
 * @param int_no Interrupt number (start at 0)
 * @param handler A handler. This should return 0 on success, anything else panics.
 *                It will take an exception number, irq number, registers, and extended registers as arguments.
 * @returns 0 on success, -EINVAL if handler is taken
 */
int hal_registerInterruptHandler(uintptr_t int_no, interrupt_handler_t handler) {
    if (hal_handler_table[int_no] != NULL) {
        return -EINVAL;
    }

    hal_handler_table[int_no] = handler;
    hal_interrupt_context_table[int_no] = NULL;

    return 0;
}

/**
 * @brief Unregisters an interrupt handler
 */
void hal_unregisterInterruptHandler(uintptr_t int_no) {
    hal_handler_table[int_no] = NULL;
}

/**
 * @brief Register an exception handler
 * @param int_no Exception number
 * @param handler A handler. This should return 0 on success, anything else panics.
 *                It will take an exception number, registers, and extended registers as arguments.
 * @returns 0 on success, -EINVAL if handler is taken
 */
int hal_registerExceptionHandler(uintptr_t int_no, exception_handler_t handler) {
    if (hal_exception_handler_table[int_no] != NULL) {
        return -EINVAL;
    }

    hal_exception_handler_table[int_no] = handler;

    return 0;
}

/**
 * @brief Unregisters an exception handler
 */
void hal_unregisterExceptionHandler(uintptr_t int_no) {
    hal_exception_handler_table[int_no] = NULL;
}

/**
 * @brief Register an interrupt handler with context
 * @param int_no The interrupt number
 * @param handler The handler for the interrupt (should accept context)
 * @param context The context to pass to the handler
 * @returns 0 on success, -EINVAL if taken
 */
int hal_registerInterruptHandlerContext(uintptr_t int_no, interrupt_handler_context_t handler, void *context) {
    if (hal_handler_table[int_no] != NULL) {
        return -EINVAL;
    }

    // !!!: i mean, they're the same data type at the core level.. right?
    hal_handler_table[int_no] = handler;
    hal_interrupt_context_table[int_no] = context;

    return 0;
}

/**
 * @brief Register a vector in the IDT table.
 * @warning THIS IS FOR INTERNAL USE ONLY. @see hal_registerInterruptHandler for
 *          a proper handler register 
 * 
 * @note This isn't static because some more advanced functions will need
 *       to set vectors up as usermode (with differing CPL/DPL)
 * 
 * @param index Index in the kernel IDT table (side note: this value cannot be wrong.)
 * @param flags The flags. Recommended to use I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32 for kernel mode,
 *              although you can OR by I86_IDT_DESC_RING3 for usermode
 * @param segment The segment selector. Should always be 0x08.
 * @param base The interrupt handler
 *
 * @returns 0. Anything else, and you're in for a REALLY bad day. 
 */
int hal_registerInterruptVector(uint8_t index, uint8_t flags, uint16_t segment, uint32_t base) {
    hal_idt_table[index].base_lo = base & 0xFFFF;
    hal_idt_table[index].base_hi = (base >> 16) & 0xFFFF;

    hal_idt_table[index].segment_selector = segment;
    hal_idt_table[index].flags = flags;

    return 0;
}

/**
 * @brief Initialize the 8259 PIC(s)
 * Uses default offsets 0x20 for master and 0x28 for slave
 */
void hal_initializePIC() {
    uint8_t master_mask, slave_mask;

    // Read & save the PIC masks
    master_mask = inportb(I86_PIC1_DATA);
    slave_mask = inportb(I86_PIC2_DATA);

    // Begin initialization sequence in cascade mode
    outportb(I86_PIC1_COMMAND, I86_PIC_ICW1_INIT | I86_PIC_ICW1_ICW4); 
    io_wait();
    outportb(I86_PIC2_COMMAND, I86_PIC_ICW1_INIT | I86_PIC_ICW1_ICW4);
    io_wait();

    // Send offsets
    outportb(I86_PIC1_DATA, 0x20);
    io_wait();
    outportb(I86_PIC2_DATA, 0x28);
    io_wait();

    // Identify slave PIC to be at IRQ2
    outportb(I86_PIC1_DATA, 4);
    io_wait();
    
    // Notify slave PIC of cascade identity
    outportb(I86_PIC2_DATA, 2);
    io_wait();

    // Switch to 8086 mode
    outportb(I86_PIC1_DATA, I86_PIC_ICW4_8086);
    io_wait();
    outportb(I86_PIC2_DATA, I86_PIC_ICW4_8086);  
    io_wait();

    // Restore old masks
    outportb(I86_PIC1_DATA, master_mask);
    outportb(I86_PIC2_DATA, slave_mask);
}

/**
 * @brief Disable the 8259 PIC(s)
 */
void hal_disablePIC() {
    outportb(I86_PIC1_DATA, 0xFF);
    outportb(I86_PIC2_DATA, 0xFF);
}

/**
 * @brief Installs the IDT in the current AP
 */
void hal_installIDT() {
    // Install the IDT
    i386_idtr_t idtr; 
    idtr.base = (uint32_t)hal_idt_table;
    idtr.limit = sizeof(hal_idt_table) - 1;
    asm volatile("lidt %0" :: "m"(idtr));
}

/**
 * @brief Initialize HAL interrupts (IDT, GDT, TSS, etc.)
 */
void hal_initializeInterrupts() {
    // Start the GDT
    hal_installGDT();

    // Clear the IDT table
    memset((void*)hal_idt_table, 0x00, sizeof(hal_idt_table));

    // Install the handlers
    hal_registerInterruptVector(0, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halDivisionException);
    hal_registerInterruptVector(1, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halDebugException);
    hal_registerInterruptVector(2, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halNMIException);
    hal_registerInterruptVector(3, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halBreakpointException);
    hal_registerInterruptVector(4, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halOverflowException);
    hal_registerInterruptVector(5, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halBoundException);
    hal_registerInterruptVector(6, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halInvalidOpcodeException);
    hal_registerInterruptVector(7, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halNoFPUException);
    hal_registerInterruptVector(8, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halDoubleFaultException);
    hal_registerInterruptVector(9, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halCoprocessorSegmentException);
    hal_registerInterruptVector(10, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halInvalidTSSException);
    hal_registerInterruptVector(11, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halSegmentNotPresentException);
    hal_registerInterruptVector(12, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halStackSegmentException);
    hal_registerInterruptVector(13, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halGeneralProtectionException);
    hal_registerInterruptVector(14, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halPageFaultException);
    hal_registerInterruptVector(15, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halReservedException);
    hal_registerInterruptVector(16, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halFloatingPointException);
    hal_registerInterruptVector(17, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halAlignmentCheck);
    hal_registerInterruptVector(18, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halMachineCheck);
    hal_registerInterruptVector(19, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halSIMDFloatingPointException);
    hal_registerInterruptVector(20, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halVirtualizationException);
    hal_registerInterruptVector(21, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halControlProtectionException);
    hal_registerInterruptVector(28, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halHypervisorInjectionException);
    hal_registerInterruptVector(29, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halVMMCommunicationException);
    hal_registerInterruptVector(30, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halSecurityException);
    hal_registerInterruptVector(31, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halReserved2Exception);

    hal_registerInterruptVector(32, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ0);
    hal_registerInterruptVector(33, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ1);
    hal_registerInterruptVector(34, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ2);
    hal_registerInterruptVector(35, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ3);
    hal_registerInterruptVector(36, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ4);
    hal_registerInterruptVector(37, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ5);
    hal_registerInterruptVector(38, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ6);
    hal_registerInterruptVector(39, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ7);
    hal_registerInterruptVector(40, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ8);
    hal_registerInterruptVector(41, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ9);
    hal_registerInterruptVector(42, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ10);
    hal_registerInterruptVector(43, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ11);
    hal_registerInterruptVector(44, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ12);
    hal_registerInterruptVector(45, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ13);
    hal_registerInterruptVector(46, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ14);
    hal_registerInterruptVector(47, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32, 0x08, (uint32_t)&halIRQ15);

    // Install IDT in BSP
    hal_installIDT();

    // Setup the PIC
    hal_initializePIC();

    // Enable interrupts
    __asm__ __volatile__("sti");

    dprintf(INFO, "Interrupts enabled successfully\n");
}