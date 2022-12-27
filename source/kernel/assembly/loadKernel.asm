[bits 32]
[extern kmain]


[global _start]
_start:
    xor ebp, ebp
    push eax
    call kmain
    jmp $
