; reduceOS x86_64 multiboot loader - written by sasdallas
; This file conforms to Multiboot 1 specifications.


; Starting heading
global _start
extern long_entry


; **** MULTIBOOT HEADING ****
extern bss_start
extern end
extern start

; Flags
PG_ALIGN            equ 1 << 0
MEMINFO             equ 1 << 1


MULTIBOOT_FLAGS equ PG_ALIGN | MEMINFO
MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

KERNEL_MAGIC equ 0x43D8C305 ; reduceOS (it sort of looks like it)

section .multiboot 
multiboot_header:
    align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

    ; GRUB wants this info, so give it to it.
    dd multiboot_header
    dd start
    dd bss_start
    dd end
    dd _start

section .data
    align 4096

section .bss
    align 16
    stack_bottom:
        resb 16384 ; 16 KiB
    stack_top:


; **** CODE START ****

section .text
bits 32
_start:
    mov esp, stack_top

    ; Okay, so, now that we're here we need to trampoline into long mode
    ; But first we should actually check if long mode is available 
    call CheckCPUID
    call CheckLongMode
    call SetupPaging
    
    ; Let's set the long mode bit in the EFER MSR
    mov ecx, 0xC0000080         ; EFER MSR
    rdmsr
    or eax, 1 << 8              ; LM bit is the 9th bit
    wrmsr

    ; Before we enable paging, there's a possibility that we already had it enabled
    mov eax, cr0
    test eax, 0x80000000
    jnz .LongModeP2
    

    ; Now let's enable paging and protected mode
    mov eax, cr0
    or eax, 1 << 31 | 1 << 0
    mov cr0, eax

.LongModeP2:
    ; Now we're in compatibility mode, we need to load the GDT and jump to the long mode entrypoint
    lgdt [GDT.Pointer]
    
    ; Jump to it!
    jmp GDT.Code:long_entry

    ; wtf
    mov al, '?'
    jmp BootError


CheckCPUID:
    ; Check if the CPU supports the CPUID instruction
    ; We'll do this by trying to flip the ID bit (bit 21) in EFLAGS
    ; If the bit can be flipped, then we're good to go.
    
    ; Copy EFLAGS into EAX
    pushfd
    pop eax

    ; Copy to ECX too, we'll use it later
    mov ecx, eax

    ; Flip the ID bit
    xor eax, 1 << 21

    ; Restore EFLAGS
    push eax
    popfd           

    ; Copy FLAGS back to EAX (again)
    pushfd
    pop eax

    ; Restore EFLAGS from the old version stored in ECX
    push ecx
    popfd

    ; Compare EAX and ECX to see if they're equal
    ; If they are, the bit wasn't flipped
    xor eax, ecx
    jz .NoCPUID
    ret

.NoCPUID:
    ; Our error handling method takes in a single character
    mov al, 'C'
    jmp BootError


CheckLongMode:
    ; Long mode can only be checked via the CPUID extended instruction
    ; We'll have to check if that's supported to
    mov eax, 0x80000000         ; CPUID type
    cpuid
    cmp eax, 0x80000001
    jb .NoLongMode              ; Unsupported :(

    ; Now we'll actually check if long mode is supported
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .NoLongMode
    ret

.NoLongMode:
    mov al, 'L'
    jmp BootError


SetupPaging:
    ; Disable paging if it was set up by clearing the PG bit
    mov eax, cr0
    and eax, 01111111111111111111111111111111b 
    mov cr0, eax

    ; Long mode uses PAE paging, which consists of a few parts like PDPT, PDT, PT, PML4T.
    ; More details:
    ; - PDPT: Page-directory pointer table
    ; - PDT: Page-directory table
    ; - PT: Page table
    ; - PML4T: Page-map level 4 table 
    ; Hierarchy: PML4T -> PDPT -> PDT -> PT -> Physical Address
    ; Each table has 512 entries (0 indexed) which all refer to a specific region of memory
    ; We'll start by setting up four tables at 0x1000

    ; Let's start by clearing the table
    mov edi, 0x1000             ; EDI is the destination index
    mov cr3, edi                ; Set CR3 to our destination index
    xor eax, eax                ; Clear EAX
    mov ecx, 4096
    rep stosd                   ; Clear the memory
    mov edi, cr3                ; Set EDI to CR3

    ; Now let's setup the tables
    mov dword [edi], 0x2003     ; Set the destination index of the PML4T to 0x2003
    add edi, 0x1000
    mov dword [edi], 0x3003     ; Set the destination index of the PDPT to 0x3003
    add edi, 0x1000
    mov dword [edi], 0x4003     ; Set the destination index of the PDT to 0x4003
    add edi, 0x1000

    ; Now we'll need to identity map the first 2MB
    mov ebx, 0x00000003
    mov ecx, 512

.SetEntry:
    mov dword [edi], ebx
    add ebx, 0x1000
    add edi, 8
    loop .SetEntry

    ; Now we'll enable PAE paging via CR4
    mov eax, cr4 
    or eax, 1 << 5
    mov cr4, eax

    ; Paging is now ready, let's return
    ret

BootError:
    ; Print the string "ERROR: " and the character in AL
    mov dword   [0xb8000], 0x4f524f45
    mov dword   [0xb8004], 0x4f4f4f52
    mov dword   [0xb8008], 0x4f3a4f52
    mov dword   [0xb800c], 0x4f204f20
    mov byte    [0xb800e], al
.halt:
    hlt
    jmp .halt


; GDT
PRESENT        equ 1 << 7
NOT_SYS        equ 1 << 4
EXEC           equ 1 << 3
DC             equ 1 << 2
RW             equ 1 << 1
ACCESSED       equ 1 << 0

; Flags bits
GRAN_4K       equ 1 << 7
SZ_32         equ 1 << 6
LONG_MODE     equ 1 << 5

GDT:
    .Null: equ $ - GDT
        dq 0
    .Code: equ $ - GDT
        dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
        db 0                                        ; Base (mid, bits 16-23)
        db PRESENT | NOT_SYS | EXEC | RW            ; Access
        db GRAN_4K | LONG_MODE | 0xF                ; Flags & Limit (high, bits 16-19)
        db 0                                        ; Base (high, bits 24-31)
    .Data: equ $ - GDT
        dd 0xFFFF                                   ; Limit & Base (low, bits 0-15)
        db 0                                        ; Base (mid, bits 16-23)
        db PRESENT | NOT_SYS | RW                   ; Access
        db GRAN_4K | SZ_32 | 0xF                    ; Flags & Limit (high, bits 16-19)
        db 0                                        ; Base (high, bits 24-31)
    .TSS: equ $ - GDT
        dd 0x00000068
        dd 0x00CF8900
    .Pointer:
        dw $ - GDT - 1
        dq GDT