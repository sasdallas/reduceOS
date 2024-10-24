[bits 32]


ENABLE_PAGING equ 0x80000001
DISABLE_PAGING equ 0x7FFFFFFF


%define REBASE_ADDRESS(x)  (0x7c00 + ((x) - BIOS32_START))

section .text
    global BIOS32_START ; start memory address
    global BIOS32_END  ; end memory address

    global bios32_gdt_ptr  ; GDT pointer
    global bios32_gdt_entries ; GDT array entries
    global bios32_idt_ptr ; IDT pointer
    global bios32_in_reg16_ptr ; IN REGISTERS16 
    global bios32_out_reg16_ptr ; OUT REGISTERS16
    global bios32_int_number_ptr ; bios interrupt number to be called


; 32 bit protected mode
BIOS32_START:use32
    ; Save registers to stack (EFLAGS & registers)
    pushf
    pusha

    ; Save the original IDTR and GDTR onto the stack
    sub esp, 32
    sidt [esp]
    sgdt [esp+16]

    ; Save protected mode ESP for context resuming
    mov [REBASE_ADDRESS(prot_esp)], esp

    ; Disable interrupts so we're not interrupted
    cli

    ; Turn off paging
    mov eax, cr0
    and eax, DISABLE_PAGING
    mov cr0, eax

    ; Flush TLB
    mov eax, cr3
    mov cr3, eax

    ; Load GDT
    lgdt [REBASE_ADDRESS(bios32_gdt_ptr)]

    ; Load IDT
    lidt [REBASE_ADDRESS(bios32_idt_ptr)]

    ; Jump to 16-bit protected mode
    jmp 0x30:REBASE_ADDRESS(__protected_mode_16)

; 16 bit protected mode
__protected_mode_16:use16
    ; Prepare to jump to 16-bit real mode
    mov ax, 0x38
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Disable PE in CR0
    mov eax, cr0
    and al,  ~0x01
    mov cr0, eax
    jmp 0x0:REBASE_ADDRESS(__real_mode_16)

; 16 bit real mode
__real_mode_16:use16
    ; Clear segment registers (real mode)
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov sp, 0x8c00 ; Stack at 0x8c00
    
    ; We can't enable STI because IRQs haven't been remapped and external interrupts will fail.
    ; TODO: Use custom IDT? Maybe find the old BIOS one?
    ; stiw

    ; Save real mode context
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

    ; Store stack
    mov eax, esp
    mov edi, REBASE_ADDRESS(current_esp)
    stosd

    ; Load stack to input registers
    mov sp, REBASE_ADDRESS(bios32_in_reg16_ptr)

    ; TODO: We are only using general registers, meaning any changes to EFLAGS and such will break. Is this bad?
    popa

    ; Might've messed with the stack, clean that up.
    mov sp, 0x9c00
    
    ; CD = i386 INT opcode
    db 0xCD

bios32_int_number_ptr: ; BIOS32 interrupt number
    ; Overwritten with interrupt number
    db 0x00

    ; Output context stored here
    mov esp, REBASE_ADDRESS(bios32_out_reg16_ptr)
    add sp, 28 ; Restore stack
   
    ; Save current context (output)
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
    mov esi, REBASE_ADDRESS(current_esp)
    lodsd
    mov esp, eax

    ; Restore old context
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

    ; Moving back to protected mode
    cli
    mov eax, cr0
    or eax, ENABLE_PAGING
    mov cr0, eax

    ; Jump!
    jmp 0x08:REBASE_ADDRESS(__protected_mode_32)

; 32 bit protected mode
__protected_mode_32:use32
    ; Reset data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Restore ESP
    mov esp, [REBASE_ADDRESS(prot_esp)]

    ; Reload the original IDTR and GDTR from the stack
    lidt [esp]
    lgdt [esp+16]
    add esp, 32

    ; Restore all registers
    popa

    ; Restore EFLAGS
    popf
    ret

align 4

bios32_gdt_entries:
    ; 8 gdt entries
    TIMES 8 dq 0

bios32_gdt_ptr:
    dd 0x00000000
    dd 0x00000000

bios32_idt_ptr:
    dd 0x00000000
    dd 0x00000000

bios32_in_reg16_ptr:
    TIMES 14 dw 0

bios32_out_reg16_ptr:
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
    dd 0xaaaaaaaa
current_esp:
    dd 0x0000
prot_esp:
    dd 0x0000

BIOS32_END:
