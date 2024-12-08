/**
 * @file hexahedron/arch/i386/smp_trampoline.S
 * @brief AP initial startup file
 * 
 * This will get the APs ready for 32-bit protected mode and prepare any data needed.
 * NOTE: This file is based off of predetermined addresses and load sections.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

.global _ap_parameters
.global _ap_startup
.global _ap_gdtr
.global _ap_bootstrap_start
.global _ap_bootstrap_end
.extern smp_finalizeAP
.section .ap_bootstrap

.code16

_ap_bootstrap_start:

_ap_startup:    /* 0x1000 */
    cli
    cld
    ljmp $0, $0x1040

.align 16
_ap_gdt_table:  /* 0x1010 */
    .long 0, 0
    .long 0x0000FFFF, 0x00CF9A00    // 32-bit code segment
    .long 0x0000FFFF, 0x008F9200    // 32-bit data segment
    .long 0x00000068, 0x00CF8900    // tss
_ap_gdtr:       /* 0x1030 */
    .word _ap_gdtr - _ap_gdt_table - 1
    .long 0x1010
    .long 0, 0

.align 64
_ap_startup_real:   /* 0x1040 */
    /* Load GDT */
    xchgw %bx, %bx
    lgdtl 0x1030

    /* Prepare to switch to protected mode */
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    /* Jump to protected mode! */
    ljmp $0x08, $0x1060

.align 32
.code32


_ap_startup_pmode:  /* 0x1060 */
    /* Setup segments */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
     
    /* Load the stack */
    movl $0x1000 + _ap_stack - _ap_bootstrap_start, %esp

    /* We're good enough to jump to smp_finalizeAP */
    pushl $0x1000 + _ap_parameters - _ap_bootstrap_start
    ljmp $0x08, $smp_finalizeAP

    cli
    hlt
    jmp .


/* C code will copy this */
_ap_parameters:
_ap_stack:
    .long 0       
_ap_pagedir:
    .long 0
_ap_lapic_id:
    .long 0


_ap_bootstrap_end:
