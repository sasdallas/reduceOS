; tss.asm - initializes the task state segment


global tssFlush

; tssFlush - install the tss
tssFlush:
    mov ax, 0x28
    ltr ax
    ret