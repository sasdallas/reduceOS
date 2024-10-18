[bits 32]
[extern isrExceptionHandler]
[extern isrIRQHandler]


isrCommonStub:
    pusha                           ; Push all generic regisers

    mov ax, ds
    push eax

    mov ax, 0x10                    ; Setup kernel data segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                        ; Push stack pointer
    call isrExceptionHandler
    pop eax                         ; EAX = ESP

    pop ebx                         ; Restore DS code selectors
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    popa                            ; Restore generic registers
    add esp, 8                      ; Skip over int_no and err_code
    iret                            ; Jump to EIP

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
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    popa
    add esp, 8
    iret



%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        push byte 0 ; Dummy error code
        push %1
        jmp isrCommonStub
%endmacro

%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        push %1 
        jmp isrCommonStub
%endmacro

%macro IRQ 2
    global irq_%1
    irq_%1:
        push byte %1
        push %2
        jmp irqCommonStub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20

ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

ISR_NOERRCODE 128

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39 
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

