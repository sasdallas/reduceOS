; reduceOS multiboot loader - written by sasdallas
; This file conforms to Multiboot 1 specifications.

; isn't this also like the 834th time we've flipped between AT&T syntax and NASM?

extern bss_start
extern end
extern start

; Flags
PG_ALIGN            equ 1 << 0
MEMINFO             equ 1 << 1
VESA_FRAMEBUFFER    equ 1 << 2


MULTIBOOT_FLAGS equ PG_ALIGN | MEMINFO | VESA_FRAMEBUFFER
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

    ; Request a framebuffer for 1024x768 with colordepth 32 BPP
    dd 0x0000000
    dd 1024
    dd 768
    dd 32

section .data
    align 4096

section .bss
    align 16
    stack_bottom:
        resb 16384 ; 16 KiB
    stack_top:

section .text
global _start
extern kmain
_start:
    mov esp, stack_top
    push ebp ; Stack trace

    mov eax, KERNEL_MAGIC
    push eax
    push ebx
    call kmain

    jmp $ ; Infinite loop