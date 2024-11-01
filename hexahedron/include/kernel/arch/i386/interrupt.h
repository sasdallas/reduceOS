/**
 * @file hexahedron/include/kernel/arch/i386/interrupt.h
 * @brief Interrupt declarations for the I386 architecture
 * 
 * Implements basic structures for things like GDT/IDT
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_ARCH_I386_INTERRUPT_H
#define KERNEL_ARCH_I386_INTERRUPT_H

/**** INCLUDES ****/
#include <stdint.h>

/**** TYPES ****/

typedef struct _i386_interrupt_descriptor {
    uint16_t base_lo;               // Low 16-bits of interrupt routine address
    uint16_t segment_selector;      // Code segment selector (in GDT)
    uint8_t reserved;               // Reserved
    uint8_t flags;                  // Gate type, DPL, P fields, etc.
    uint16_t base_hi;               // High 16-bits of interrupt routine address
} i386_interrupt_descriptor_t;

typedef struct _i386_idtr {
    uint16_t limit;
    uint32_t base;
} i386_idtr_t;

/* GDT structures are unused for now. They were planned to be used but are now vestigal. */

// This structure is ugly to implement
typedef struct _i386_gdt_descriptor {
    uint16_t limit;                 // Maximum address
    uint16_t base_lo;               // Lower 16 bits of the base
    uint8_t base_mid;               // Next 8 bits of the base
    uint8_t access;                 // Access bits (determines segment ring)
    uint8_t flags;                  // Should contain flags, but also contains part of the 20-bit limit address.
                                    // Why would you do this, Intel?
    uint8_t base_hi;                // Final 8 bits of base
} __attribute__((packed)) i386_gdt_descriptor_t;

typedef struct _i386_gdtr {
    uint16_t limit;
    uint32_t base;
} i386_gdtr_t;

/**** DEFINITIONS ****/

// Copied straight from old kernel. Provide interrupt descriptor types.
#define I86_IDT_DESC_BIT16 0x06     // 00000110
#define I86_IDT_DESC_BIT32 0x0E     // 00001110
#define I86_IDT_DESC_RING1 0x40     // 01000000
#define I86_IDT_DESC_RING2 0x20     // 00100000
#define I86_IDT_DESC_RING3 0x60     // 01100000
#define I86_IDT_DESC_PRESENT 0x80   // 10000000

// PIC definitions
#define I86_PIC1_ADDR       0x20                // Master PIC address
#define I86_PIC2_ADDR       0xA0                // Slave PIC address
#define I86_PIC1_COMMAND    (I86_PIC1_ADDR)     // Command address for master PIC
#define I86_PIC2_COMMAND    (I86_PIC2_ADDR)     // Command address for slave PIC
#define I86_PIC1_DATA       (I86_PIC1_ADDR)+1   // Data address for master PIC
#define I86_PIC2_DATA       (I86_PIC2_ADDR)+1   // Data address for slave PIC

#define I86_PIC_EOI             0x20        // End of interrupt code

// PIC ICW (initialization words)
#define I86_PIC_ICW1_ICW4       0x01        // Indicates that ICW4 is present
#define I86_PIC_ICW1_SINGLE     0x02        // Enables PIC cascade mode
#define I86_PIC_ICW1_INTERVAL4  0x04        // Call address interval 4
#define I86_PIC_ICW1_LEVEL      0x08        // Edge mode
#define I86_PIC_ICW1_INIT       0x10        // Initialization bit

#define I86_PIC_ICW4_8086       0x01        // Enables 8086/88 mode
#define I86_PIC_ICW4_AUTO       0x02        // Auto EOI 
#define I86_PIC_ICW4_BUF_SLAVE  0x08        // Enable buffered mode (slave)
#define I86_PIC_ICW4_BUF_MASTER 0x0C        // Enable buffered mode (master)
#define I86_PIC_ICW4_SFNM       0x10        // Special fully nested

#endif