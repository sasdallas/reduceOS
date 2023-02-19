
global switchToUserMode

switchToUserMode:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    push esp
    pushfd

    ; Code segment selector (RPL 3)
    push 0x1B
    lea eax, [userStart]
    push eax
    iretd
userStart:
    add esp, 4
    ret
    