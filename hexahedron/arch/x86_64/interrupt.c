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
 * @brief Initializes the PIC, GDT/IDT, TSS, etc.
 */
void hal_initializeInterrupts() {
    // Start the GDT
    hal_gdtInit();

    
}