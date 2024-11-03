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

.extern hal_exceptionHandler

/* Save registers macro */
.macro PUSH_REGISTERS
    /* This is pushing registers_t */
    pusha

    pushl %ds
    pushl %es
    pushl %fs

    /* This is pushing extended regiisters */
    movl %cr0, %eax
    pushl %eax
    movl %cr2, %eax
    push %eax
    movl %cr3, %eax
    pushl %eax
    movl %cr4, %eax
    push %eax

    // Toss the stack to below the idtr (uint16 + uint32 = 6 bytes * 2 = 12 bytes)
    sub $12, %esp
    sidt (%esp)
    sgdt 6(%esp)
.endm


/* Common exception handler */
halCommonExceptionHandler:
    PUSH_REGISTERS // Push registers

    leal 28(%esp), %eax
    pushl %esp
    pushl %eax
    push $halExceptionIndex
    call hal_exceptionHandler

    cli
    hlt

/* Common IRQ handler */
halCommonIRQHandler:
    cli
    hlt


/* Exception macros */
.macro ISR_NOERRCODE name index
    .global \name
    &name:
        push 0 // Push dummy error code
        movl $\index, halExceptionIndex
        jmp halCommonExceptionHandler
.endm

.macro ISR_ERRCODE name index
    .global \name
    &name:
        movl $\index, halExceptionIndex
        jmp halCommonExceptionHandler
.endm

/* IRQ macro */
.macro IRQ name index
    .global \name 
    &name:
        movl $\index, halExceptionIndex
        movl $\index - 32, halIRQIndex 
        jmp halCommonIRQHandler
.endm



/* Handler definitions */
ISR_NOERRCODE   halDivisionException,               0
ISR_NOERRCODE   halDebugException,                  1
ISR_NOERRCODE   halNMIException,                    2
ISR_NOERRCODE   halBreakpointException,             3
ISR_NOERRCODE   halOverflowException,               4
ISR_NOERRCODE   halBoundException,                  5
ISR_NOERRCODE   halInvalidOpcodeException,          6
ISR_NOERRCODE   halNoFPUException,                  7
ISR_ERRCODE     halDoubleFaultException,            8
ISR_NOERRCODE   halCoprocessorSegmentException,     9 // If you get this, god help you. This is reserved.
ISR_ERRCODE     halInvalidTSSException,             10
ISR_ERRCODE     halSegmentNotPresentException,      11
ISR_ERRCODE     halStackSegmentException,           12
ISR_ERRCODE     halGeneralProtectionException,      13
ISR_ERRCODE     halPageFaultException,              14
ISR_NOERRCODE   halReservedException,               15
ISR_NOERRCODE   halFloatingPointException,          16
ISR_ERRCODE     halAlignmentCheck,                  17
ISR_NOERRCODE   halMachineCheck,                    18
ISR_NOERRCODE   halSIMDFloatingPointException,      19
ISR_NOERRCODE   halVirtualizationException,         20
ISR_ERRCODE     halControlProtectionException,      21
/* 22 - 27 are reserved */
ISR_NOERRCODE   halHypervisorInjectionException,    28 // mfw hypervisor injection exception
ISR_ERRCODE     halVMMCommunicationException,       29
ISR_ERRCODE     halSecurityException,               30
ISR_NOERRCODE   halReserved2Exception,              31

IRQ             halIRQ0,    32
IRQ             halIRQ1,    33
IRQ             halIRQ2,    34
IRQ             halIRQ3,    35
IRQ             halIRQ4,    36
IRQ             halIRQ5,    37
IRQ             halIRQ6,    38
IRQ             halIRQ7,    39
IRQ             halIRQ8,    40
IRQ             halIRQ9,    41
IRQ             halIRQ10,   42
IRQ             halIRQ11,   43
IRQ             halIRQ12,   44
IRQ             halIRQ13,   45
IRQ             halIRQ14,   46
IRQ             halIRQ15,   47




/* These indexes are useful because stack manipulation is hard :( */
halExceptionIndex:
    .long 0

halIRQIndex:
    .long 0


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