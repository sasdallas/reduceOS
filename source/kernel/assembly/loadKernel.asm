[bits 32]
[extern kmain]


[global _start]
_start:
    push eax
    call kmain
    jmp $
