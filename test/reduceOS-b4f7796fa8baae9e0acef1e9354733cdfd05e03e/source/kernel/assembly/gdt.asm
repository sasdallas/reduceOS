[bits 32]
[global install_gdt]

install_gdt:
    mov eax, [esp+4]                ; Get pointer to GDT, passed as a parameter
    lgdt [eax]                      ; Install GDT pointer

    mov ax, 0x10                    ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax                      ; Load all data segment registers
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush                 ; 0x08 is the offset to our code segment.
.flush:
    ret
    