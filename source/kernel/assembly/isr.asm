[bits 32]
[extern isrExceptionHandler]
[extern isrIRQHandler]
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31
global isr128
global irq_0
global irq_1
global irq_2
global irq_3
global irq_4
global irq_5
global irq_6
global irq_7
global irq_8
global irq_9
global irq_10
global irq_11
global irq_12
global irq_13
global irq_14
global irq_15



isrCommonStub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isrExceptionHandler
    pop eax

    pop ds
    pop es
    popa
    add esp, 8
    iret

irqCommonStub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isrIRQHandler
    pop eax

    pop ebx
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa

    add esp, 8

    iret

; isr0 - div by 0 exception
isr0:
    push byte 0
    push 0
    jmp isrCommonStub


; isr1 - debug
isr1:
    push byte 0
    push 1
    jmp isrCommonStub

; isr2 - non-maskable interrupt
isr2:
    push byte 0
    push 2
    jmp isrCommonStub


; isr3 - breakpoint
isr3:
    push byte 0
    push 3
    jmp isrCommonStub
    
    
; isr4 - overflow
isr4:
    push byte 0
    push 4
    jmp isrCommonStub


; isr5 - bound range exceeded
isr5:
    push byte 0
    push 5
    jmp isrCommonStub

; isr2 - invalid opcode
isr6:
    push byte 0
    push 6
    jmp isrCommonStub
    
    
; isr7 - device not available 
isr7:
    push byte 0
    push 7
    jmp isrCommonStub
    
    
; isr8 - double fault
isr8:
    push byte 0
    push 8
    jmp isrCommonStub
    
    
; isr9 - coprocessor segment overrun
isr9:
    push byte 0
    push 9
    jmp isrCommonStub
    
    
; isr10 - invalid TSS
isr10:
    push byte 0
    push 10
    jmp isrCommonStub
    
    
; isr11 - segment not present
isr11:
    push byte 0
    push 11
    jmp isrCommonStub
    
    
; isr12 - stack segment fault
isr12:
    push byte 0
    push 12
    jmp isrCommonStub
    
    
; isr13 - general protection
isr13:
    push byte 0
    push 13
    jmp isrCommonStub
    
    
; isr14 - page fault
isr14:
    push byte 0
    push 14
    jmp isrCommonStub
    
    
; isr15 - unknown interrupt
isr15:
    push byte 0
    push 15
    jmp isrCommonStub

; isr16 - FPU floating point exception
isr16:
    push byte 0
    push 16
    jmp isrCommonStub

; isr17 - alignment check
isr17:
    push byte 0
    push 17
    jmp isrCommonStub


; isr18 - machine check
isr18:
    push byte 0
    push 18
    jmp isrCommonStub

; isr19 - SIMD floating-point exception
isr19:
    push byte 0
    push 19
    jmp isrCommonStub

; isr20 - virtualization exception
isr20:
    push byte 0
    push 20
    jmp isrCommonStub

; isr21 - Reserved
isr21:
    push byte 0
    push 21
    jmp isrCommonStub

; isr22 - Reserved
isr22:
    push byte 0
    push 22
    jmp isrCommonStub

; isr23 - Reserved
isr23:
    push byte 0
    push 23
    jmp isrCommonStub

; isr24 - Reserved
isr24:
    push byte 0
    push 24
    jmp isrCommonStub

; isr25 - Reserved
isr25:
    push byte 0
    push 25
    jmp isrCommonStub

; isr26 - Reserved
isr26:
    push byte 0
    push 26
    jmp isrCommonStub

; isr27 - Reserved
isr27:
    push byte 0
    push 27
    jmp isrCommonStub

; isr28 - Reserved
isr28:
    push byte 0
    push 28
    jmp isrCommonStub

; isr29 - Reserved
isr29:
    push byte 0
    push 29
    jmp isrCommonStub

; isr30 - Reserved
isr30:
    push byte 0
    push 30
    jmp isrCommonStub

; isr31 - Reserved
isr31:
    push byte 0
    push 31
    jmp isrCommonStub

; isr128 - Reserved
isr128:
    cli                     ; Disable interrupts
    push byte 0
    push 281
    jmp isrCommonStub



irq_0:
	push byte 0
	push byte 32
	jmp irqCommonStub

irq_1:
	push byte 1
	push byte 33
	jmp irqCommonStub

irq_2:
	push byte 2
	push byte 34
	jmp irqCommonStub

irq_3:
	push byte 3
	push byte 35
	jmp irqCommonStub

irq_4:
	push byte 4
	push byte 36
	jmp irqCommonStub

irq_5:
	push byte 5
	push byte 37
	jmp irqCommonStub

irq_6:
	push byte 6
	push byte 38
	jmp irqCommonStub

irq_7:
	push byte 7
	push byte 39
	jmp irqCommonStub

irq_8:
	push byte 8
	push byte 40
	jmp irqCommonStub

irq_9:
	push byte 9
	push byte 41
	jmp irqCommonStub

irq_10:
	push byte 10
	push byte 42
	jmp irqCommonStub

irq_11:
	push byte 11
	push byte 43
	jmp irqCommonStub

irq_12:
	push byte 12
	push byte 44
	jmp irqCommonStub

irq_13:
	push byte 13
	push byte 45
	jmp irqCommonStub

irq_14:
	push byte 14
	push byte 46
	jmp irqCommonStub

irq_15:
	push byte 15
	push byte 47
	jmp irqCommonStub