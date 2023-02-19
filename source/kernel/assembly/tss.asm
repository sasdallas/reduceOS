; tss.asm - initializes the task state segment


[GLOBAL tssFlush]

; tssFlush - install the tss
tssFlush:
    mov ax, 0x2B
    ltr ax
    ret