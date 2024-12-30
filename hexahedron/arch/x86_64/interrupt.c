/**
 * @file hexahedron/arch/x86_64/interrupt.c
 * @brief x86_64 interrupts and exceptions handler
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/x86_64/interrupt.h>
#include <kernel/arch/x86_64/hal.h>
#include <kernel/arch/x86_64/smp.h>
#include <kernel/arch/x86_64/arch.h>
#include <kernel/debug.h>
#include <kernel/panic.h>

#include <errno.h>
#include <string.h>

/* GDT */
x86_64_gdt_t gdt[MAX_CPUS] __attribute__((used)) = {{
    // Define basic core data - the code will use hal_setupGDTCoreData on each CPU core
    // to setup TSS data, and use another function to copy it to the SMP trampoline.
    {
        {
            {0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00},       // Null entry
            {0xFFFF, 0x0000, 0x00, 0x9A, 0xAF, 0x00},       // 64-bit kernel-mode code segment
            {0xFFFF, 0x0000, 0x00, 0x92, 0xAF, 0x00},       // 64-bit kernel-mode data segment
            {0xFFFF, 0x0000, 0x00, 0xFA, 0xAF, 0x00},       // 64-bit user-mode code segment
            {0xFFFF, 0x0000, 0x00, 0xF2, 0xAF, 0x00},       // 64-bit user-mode data segment
            {0x0067, 0x0000, 0x00, 0xE9, 0x00, 0x00},       // 64-bit TSS
        },
        {0x00000000, 0x00000000}                            // Additional TSS data
    },
	{0, {0, 0, 0}, 0, {0, 0, 0, 0, 0, 0, 0}, 0, 0, 0},      // TSS
    {0x0000, 0x0000000000000000}                            // GDTR
}};

/* IDT */
x86_64_interrupt_descriptor_t hal_idt_table[X86_64_MAX_INTERRUPTS];

/* Interrupt handler table - TODO: More than one handler per interrupt? */
interrupt_handler_t hal_handler_table[X86_64_MAX_INTERRUPTS];

/* Exception handler table - TODO: More than one handler per exception? */
exception_handler_t hal_exception_handler_table[X86_64_MAX_EXCEPTIONS];

/* String table for exceptions */
const char *hal_exception_table[X86_64_MAX_EXCEPTIONS] = {
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
 * @brief Sets up a core's data in the global GDT
 * @param core The core number to setup
 */
static void hal_setupGDTCoreData(int core) {
    if (core > MAX_CPUS) return;

    // Copy the core's data from BSP
    if (core != 0) memcpy(&gdt[core], &gdt[0], sizeof(*gdt));

    // Setup the GDTR
    gdt[core].gdtr.limit = sizeof(gdt[core].table.entries) + sizeof(gdt[core].table.tss_extra) - 1;
    gdt[core].gdtr.base = (uintptr_t)&gdt[core].table.entries;

    // Configure the TSS entry
    uintptr_t tss = (uintptr_t)&gdt[core].tss;
    gdt[core].table.entries[5].limit = sizeof(gdt[core].tss);
    gdt[core].table.entries[5].base_lo = (tss & 0xFFFF);
    gdt[core].table.entries[5].base_mid = (tss >> 16) & 0xFF;
    gdt[core].table.entries[5].base_hi = (tss >> 24) & 0xFF;
    gdt[core].table.tss_extra.base_higher = (tss >> 32) & 0xFFFFFFFFF;
}

/**
 * @brief Initializes and installs the GDT
 */
void hal_gdtInit() {
    // For every CPU core setup its data
    for (int i = 0; i < MAX_CPUS; i++) hal_setupGDTCoreData(i);

    // Setup the TSS' RSP to point to our top of the stack
    extern void *__stack_top;
    gdt[0].tss.rsp[0] = (uintptr_t)&__stack_top;

    // Load and install
    asm volatile (
        "lgdt %0\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        "movw $0x28, %%ax\n" // 0x28 = 6th entry in the GDT (TSS)
        "ltr %%ax\n"
        :: "m"(gdt[0].gdtr) : "rax"
    ); 
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
 * @param flags The flags. Recommended to use X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32 for kernel mode,
 *              although you can OR by X86_64_IDT_DESC_RING3 for usermode
 * @param segment The segment selector. Should always be 0x08.
 * @param base The interrupt handler
 *
 * @returns 0. Anything else, and you're in for a REALLY bad day. 
 */
int hal_registerInterruptVector(uint8_t index, uint8_t flags, uint16_t segment, uint64_t base) {
    hal_idt_table[index].base_lo = (base & 0xFFFF);
    hal_idt_table[index].base_mid = (base >> 16) & 0xFFFF;
    hal_idt_table[index].base_hi = (base >> 32) & 0xFFFFFFFF;
    hal_idt_table[index].selector = segment;
    hal_idt_table[index].flags = flags;
    hal_idt_table[index].reserved = 0;
    hal_idt_table[index].ist = 0;

    return 0;
}


/**
 * @brief Handle ending an interrupt
 */
void hal_endInterrupt(uintptr_t interrupt_number) {
    if (interrupt_number > 8) outportb(X86_64_PIC2_COMMAND, X86_64_PIC_EOI);
    outportb(X86_64_PIC1_COMMAND, X86_64_PIC_EOI);
}

/**
 * @brief Common exception handler
 */
void hal_exceptionHandler(uintptr_t exception_index, registers_t *regs, extended_registers_t *regs_extended) {
    // Looks like no one caught this exception.
    kernel_panic_prepare(CPU_EXCEPTION_UNHANDLED);

    if (exception_index == 14) {
        uintptr_t page_fault_addr = 0x0;
        asm volatile ("movq %%cr2, %0" : "=a"(page_fault_addr));
        dprintf(NOHEADER, "*** ISR detected exception: Page fault at address 0x%016llX\n\n", page_fault_addr);
    } else if (exception_index < X86_64_MAX_EXCEPTIONS) {
        dprintf(NOHEADER, "*** ISR detected exception %i - %s\n\n", exception_index, hal_exception_table[exception_index]);
    } else {
        dprintf(NOHEADER, "*** ISR detected exception %i - UNKNOWN TYPE\n\n", exception_index);
    }
    
    

    dprintf(NOHEADER, "\033[1;31mFAULT REGISTERS:\n\033[0;31m");

    dprintf(NOHEADER, "RAX %016x RBX %016x RCX %016x RDX %016x\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    dprintf(NOHEADER, "RDI %016x RSI %016x RBP %016x RSP %016x\n", regs->rdi, regs->rsi, regs->rbp, regs->rsp);
    dprintf(NOHEADER, "R8  %016x R9  %016x R10 %016x R11 %016x\n", regs->r8, regs->r9, regs->r10, regs->r11);
    dprintf(NOHEADER, "R12 %016x R13 %016x R14 %016x R15 %016x\n", regs->r12, regs->r13, regs->r14, regs->r15);
    dprintf(NOHEADER, "ERR %016x RIP %016x RFL %016x\n\n", regs->err_code, regs->rip, regs->rflags);

    dprintf(NOHEADER, "CS %04x DS %04x FS %04x GS %04x SS %04x\n\n", regs->cs, regs->ds, regs->fs, regs->gs, regs->ss);
    dprintf(NOHEADER, "CR0 %08x CR2 %016x CR3 %016x CR4 %08x\n", regs_extended->cr0, regs_extended->cr2, regs_extended->cr3, regs_extended->cr4);
    dprintf(NOHEADER, "GDTR %016x %04x\n", regs_extended->gdtr.base, regs_extended->gdtr.limit);
    dprintf(NOHEADER, "IDTR %016x %04x\n", regs_extended->idtr.base, regs_extended->idtr.limit);

    // !!!: not conforming (should call kernel_panic_finalize) but whatever
    // We want to do our own traceback.
    arch_panic_traceback(10, regs);

    // Display message
    dprintf(NOHEADER, COLOR_CODE_RED "\nThe kernel will now permanently halt. Connect a debugger for more information.\n");

    // Disable interrupts & halt
    asm volatile ("cli\nhlt");
    for (;;);

    // kernel_panic_finalize();

    for (;;);
}


/**
 * @brief Common interrupt handler
 */
void hal_interruptHandler(uintptr_t exception_index, uintptr_t int_number, registers_t *regs, extended_registers_t *regs_extended) {
    hal_endInterrupt(int_number);
}

/**
 * @brief Register an interrupt handler
 * @param int_no Interrupt number
 * @param handler A handler. This should return 0 on success, anything else panics.
 *                It will take an exception number, irq number, registers, and extended registers as arguments.
 * @returns 0 on success, -EINVAL if handler is taken
 */
int hal_registerInterruptHandler(uintptr_t int_no, interrupt_handler_t handler) {
    if (hal_handler_table[int_no] != NULL) {
        return -EINVAL;
    }

    hal_handler_table[int_no] = handler;

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
 * @brief Initialize the 8259 PIC(s)
 * Uses default offsets 0x20 for master and 0x28 for slave
 */
void hal_initializePIC() {
    uint8_t master_mask, slave_mask;

    // Read & save the PIC masks
    master_mask = inportb(X86_64_PIC1_DATA);
    slave_mask = inportb(X86_64_PIC2_DATA);

    // Begin initialization sequence in cascade mode
    outportb(X86_64_PIC1_COMMAND, X86_64_PIC_ICW1_INIT | X86_64_PIC_ICW1_ICW4); 
    io_wait();
    outportb(X86_64_PIC2_COMMAND, X86_64_PIC_ICW1_INIT | X86_64_PIC_ICW1_ICW4);
    io_wait();

    // Send offsets
    outportb(X86_64_PIC1_DATA, 0x20);
    io_wait();
    outportb(X86_64_PIC2_DATA, 0x28);
    io_wait();

    // Identify slave PIC to be at IRQ2
    outportb(X86_64_PIC1_DATA, 4);
    io_wait();

    // Notify slave PIC of cascade identity
    outportb(X86_64_PIC2_DATA, 2);
    io_wait();

    // Switch to 8086 mode
    outportb(X86_64_PIC1_DATA, X86_64_PIC_ICW4_8086);
    io_wait();
    outportb(X86_64_PIC2_DATA, X86_64_PIC_ICW4_8086);  
    io_wait();
    
    // Restore old masks
    outportb(X86_64_PIC1_DATA, master_mask);
    outportb(X86_64_PIC2_DATA, slave_mask);
}

/**
 * @brief Disable the 8259 PIC(s)
 */
void hal_disablePIC() {
    outportb(X86_64_PIC1_DATA, 0xFF);
    outportb(X86_64_PIC2_DATA, 0xFF);
}

/**
 * @brief Installs the IDT in the current AP
 */
void hal_installIDT() {
    // Install the IDT
    x86_64_idtr_t idtr; 
    idtr.base = (uintptr_t)hal_idt_table;
    idtr.limit = sizeof(hal_idt_table) - 1;
    asm volatile("lidt %0" :: "m"(idtr));
}

/**
 * @brief Initializes the PIC, GDT/IDT, TSS, etc.
 */
void hal_initializeInterrupts() {
    // Start the GDT
    hal_gdtInit();

    // Clear the IDT table
    memset((void*)hal_idt_table, 0x00, sizeof(hal_idt_table));

    // Install the handlers
    hal_registerInterruptVector(0, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halDivisionException);
    hal_registerInterruptVector(1, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halDebugException);
    hal_registerInterruptVector(2, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halNMIException);
    hal_registerInterruptVector(3, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halBreakpointException);
    hal_registerInterruptVector(4, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halOverflowException);
    hal_registerInterruptVector(5, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halBoundException);
    hal_registerInterruptVector(6, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halInvalidOpcodeException);
    hal_registerInterruptVector(7, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halNoFPUException);
    hal_registerInterruptVector(8, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halDoubleFaultException);
    hal_registerInterruptVector(9, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halCoprocessorSegmentException);
    hal_registerInterruptVector(10, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halInvalidTSSException);
    hal_registerInterruptVector(11, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halSegmentNotPresentException);
    hal_registerInterruptVector(12, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halStackSegmentException);
    hal_registerInterruptVector(13, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halGeneralProtectionException);
    hal_registerInterruptVector(14, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halPageFaultException);
    hal_registerInterruptVector(15, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halReservedException);
    hal_registerInterruptVector(16, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halFloatingPointException);
    hal_registerInterruptVector(17, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halAlignmentCheck);
    hal_registerInterruptVector(18, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halMachineCheck);
    hal_registerInterruptVector(19, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halSIMDFloatingPointException);
    hal_registerInterruptVector(20, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halVirtualizationException);
    hal_registerInterruptVector(21, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halControlProtectionException);
    hal_registerInterruptVector(28, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halHypervisorInjectionException);
    hal_registerInterruptVector(29, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halVMMCommunicationException);
    hal_registerInterruptVector(30, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halSecurityException);
    hal_registerInterruptVector(31, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halReserved2Exception);

    hal_registerInterruptVector(32, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ0);
    hal_registerInterruptVector(33, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ1);
    hal_registerInterruptVector(34, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ2);
    hal_registerInterruptVector(35, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ3);
    hal_registerInterruptVector(36, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ4);
    hal_registerInterruptVector(37, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ5);
    hal_registerInterruptVector(38, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ6);
    hal_registerInterruptVector(39, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ7);
    hal_registerInterruptVector(40, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ8);
    hal_registerInterruptVector(41, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ9);
    hal_registerInterruptVector(42, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ10);
    hal_registerInterruptVector(43, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ11);
    hal_registerInterruptVector(44, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ12);
    hal_registerInterruptVector(45, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ13);
    hal_registerInterruptVector(46, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ14);
    hal_registerInterruptVector(47, X86_64_IDT_DESC_PRESENT | X86_64_IDT_DESC_BIT32, 0x08, (uint64_t)&halIRQ15);

    // Install IDT in BSP
    hal_installIDT();

    // Initialize 8259 PICs
    hal_initializePIC();

    // Enable interrupts
    asm volatile ("sti");
}