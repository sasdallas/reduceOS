/**
 * @file hexahedron/arch/x86_64/irq.S
 * @brief Handles exceptions, IRQs, etc.
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

.code64
.extern hal_exceptionHandler
.extern hal_interruptHandler

/* Swap GS base */
.macro _swapgs
    // We only need to swapgs on a usermode -> kernel mode interrupt
    cmpq $8, 24(%rsp) // 24(%rsp) = CS pushed by CPU
    je 1f
    swapgs
1:
.endm

/* Save registers macro */
.macro PUSH_REGISTERS
    // AMD killed PUSHA for some weird reason, so we'll have to
    // do this manually - and this time we go alphabetical.
    pushq %rax
    pushq %rbx 
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp

    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8

    movw %gs, %ax
    pushw %ax
    movw %fs, %ax
    pushw %ax
    movw %es, %ax
    pushw %ax

    /* This is pushing extended extended registers */
    movq %cr0, %rax
    pushq %rax
    movq %cr2, %rax
    pushq %rax
    movq %cr3, %rax
    pushq %rax
    movq %cr4, %rax
    pushq %rax

    /* Toss the stack to below the idtr (uint16 + uint64 = 10 bytes * 2 = 20 bytes) */
    sub $20, %rsp
    sidt (%rsp)
    sgdt 10(%rsp)
.endm

/* Restore registers macro */
.macro RESTORE_REGISTERS
    /* Skip over extended registers */
    add $52, %rsp

    /* Start popping registers */
    popw %ax
    movw %ax, %ds
    popw %ax
    movw %ax, %fs
    popw %ax
    movw %ax, %gs

    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx 
    popq %rax

    // Skip over error code
    add $8, %rsp
.endm

/* Common exception handler */
halCommonExceptionHandler:
    _swapgs // If usermode exception, then swapgs to get out of it
    PUSH_REGISTERS // Push registers

    leaq 52(%rsp), %rax     // Load address of registers_t

    movq halExceptionIndex, %rdi    // Exception index
    movq %rax, %rsi                 // registers_t
    movq %rsp, %rdx                 // extended_registers_t
    call hal_exceptionHandler 
    
    RESTORE_REGISTERS // Restore registers
    _swapgs // If usermode exception, then swapgs to get back in
    
    iretq   // NOTE: The q is required!


/* Common interrupt handler */
halCommonIRQHandler:
    _swapgs // If usermode exception, then swapgs to get out of it
    PUSH_REGISTERS // Push registers

    leaq 52(%rsp), %rax     // Load address of registers_t

    movq halExceptionIndex, %rdi    // Exception index
    movq halIRQIndex, %rsi          // IRQ index
    movq %rax, %rdx                 // registers_t
    movq %rsp, %rcx                 // extended_registers_t
    call hal_interruptHandler 
    
    RESTORE_REGISTERS // Restore registers
    _swapgs // If usermode exception, then swapgs to get back in
    
    iretq   // NOTE: The q is required!


/* Exception macros */
.macro ISR_NOERRCODE name index
    .global \name
    &name:
        pushq $0 // Push dummy error code
        movl $\index, halExceptionIndex
        jmp halCommonExceptionHandler
.endm

.macro ISR_ERRCODE name index
    .global \name
    &name:
        movq $\index, halExceptionIndex
        jmp halCommonExceptionHandler
.endm

/* IRQ macro */
.macro IRQ name index
    .global \name 
    &name:
        pushq $0 // Push dummy error code 
        movq $\index, halExceptionIndex
        movq $\index - 32, halIRQIndex 
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
    .quad 0

halIRQIndex:
    .quad 0