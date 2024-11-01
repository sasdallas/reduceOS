/**
 * @file hexahedron/arch/i386/irq.S
 * @brief Handles exceptions, IRQs, IDT/GDT installation, etc.
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

.code32



.global hal_installGDT
hal_installGDT:
    /* Load the GDT */
    lgdtl gdtr

    /* No idea what environment was passed, so flush data segments */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    ljmp $0x08, $hal_finishedFlushingGDT

hal_finishedFlushingGDT:
    ret



.align 8
gdtr:
    .word gdt_end - gdt_base - 1 /* GDTR limit */
    .long gdt_base

gdt_base:
    /* GDT descriptors are 8 bytes long */

    /* Null segment */
    .long 0
    .long 0

    /* Code segment - 32-bit kernel mode (0x08) */
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x9A
    .byte 0xCF
    .byte 0x00
 
    /* Data segment - 32-bit kernel mode (0x10) */
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00

    /* Code segment - 32-bit user mode (0x18) */
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0xFA
    .byte 0xCF
    .byte 0x00

    /* Data segment - 32-bit user mode (0x20) */
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0xF2
    .byte 0xCF
    .byte 0x00

gdt_end: