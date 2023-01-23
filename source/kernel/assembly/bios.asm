[bits 32]

%define BASE_ADDRESS(x) (0x7C00 + ((x) - BIOS32_START))

section .text
    global BIOS32_START                 ; Start memory address
    global BIOS32_END                   ; End memory address

    global bios32_gdtPtr                ; BIOS32 GDT pointer
    global bios32_gdtEntries            ; BIOS32 GDT entries
    global bios32_idtPtr                ; BIOS32 IDT pointer
    global bios32_inReg_ptr             ; Pointer to the IN registers.
    global bios32_outReg_ptr            ; Pointer to the OUT registers.
    global bios32_intNo_ptr             ; Pointer to the BIOS interrupt number.

; BIOS32_START - Going to protected mode
BIOS32_START:use32
    pusha                               ; Save registers
    mov edx, esp                        ; Save current ESP to EDX
    cli                                 ; Disable interrupts to move to protected mode

    ; Clear cr3 by saving cr3 in EBX
    xor ecx, ecx
    mov ebx, cr3
    mov cr3, ecx

    lgdt [BASE_ADDRESS(bios32_gdtPtr)]  ; Load new empty GDT
    lidt [BASE_ADDRESS(bios32_idtPtr)]  ; Load new empty IDT

    jmp 0x30:BASE_ADDRESS(protectedMode_16) ; Jump to 16-bit protected mode

; protectedMode_16 - 16-bit protected mode
protectedMode_16:use16
    ; Jump to 16-bit real mode.
    mov ax, 0x38
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Disable protected mode.
    mov eax, cr0
    and al, ~0x01
    mov cr0, eax
    jmp 0x0:BASE_ADDRESS(realMode_16) ; Move to real mode

; realMode_16 - 16-bit real mode.
realMode_16:use16
    ; Clear stack pointers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x8C00


    sti                              ; Enable BIOS interrupts

    ; Now, save everything.
    pusha
    mov cx, ss
    push cx
    mov cx, gs
    push cx
    mov cx, fs
    push cx
    mov cx, es
    push cx
    mov cx, ds
    push cx
    pushf

    ; Get the current stack pointer and save it to currentESP
    mov ax, sp
    mov edi, currentESP
    stosw

    ; Load the custom registers context
    mov esp, BASE_ADDRESS(bios32_inReg_ptr)

    popa

    ; Setup a new track for BIOS interrupt
    mov sp, 0x9C00

    ; Call interrupt opcode to execute
    db 0xCD

; bios32_intNo_ptr - The interrupt number passed by the C code.
bios32_intNo_ptr:
    db 0x00                     ; Interrupt number will be stored here

    ; Get output address.
    mov esp, BASE_ADDRESS(bios32_outReg_ptr)
    add sp, 28
    
    ; Save everything (again)
    pushf
    mov cx, ss
    push cx
    mov cx, gs
    push cx
    mov cx, fs
    push cx
    mov cx, es
    push cx
    mov cx, ds
    push cx
    pusha

    ; Restore ESP
    mov esi, currentESP
    lodsw
    mov sp, ax

    ; Restore everything
    popf
    pop cx
    mov ds, cx
    pop cx
    mov es, cx
    pop cx
    mov fs, cx
    pop cx
    mov gs, cx
    pop cx
    mov ss, cx
    popa

    ; jump to 32-bit protected mode
    mov eax, cr0
    inc eax
    mov cr0, eax
    jmp 0x08:BASE_ADDRESS(protectedMode_32)

protectedMode_32:use32
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Restore cr3 and esp
    mov cr3, ebx
    mov esp, ebx
    sti
    popa
    ret


padding:
    db 0x0
    db 0x0
    db 0x0

bios32_gdtEntries:
    resb 64

bios32_gdtPtr:
    dd 0x00000000
    dd 0x00000000

bios32_idtPtr:
    dd 0x00000000
    dd 0x00000000

bios32_inReg_ptr:
    resw 14

bios32_outReg_ptr:
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa

currentESP:
    dw 0x0000

BIOS32_END: