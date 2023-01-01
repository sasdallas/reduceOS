[bits 32]
[global install_idt]

install_idt:
    mov eax, [esp+4]
    lidt [eax]
    ret