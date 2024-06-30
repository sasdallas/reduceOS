; reduceOS multiboot loader - written by sasdallas
; This file conforms to Multiboot 1 specifications.

; isn't this also like the 834th time we've flipped between AT&T syntax and NASM?


; Flags
MEMORY_INFO equ 1 << 0
BOOT_DEV equ 1 << 1
CMDLINE equ 1 << 2
MODS equ 1 << 3
SYMBOL_TABLE equ 48
MEMORY_MAP equ 1 << 6
DRIVE equ 1 << 7
CONFIG_TABLE equ 1 << 8
BOOTLOADER_NAME equ 1 << 9
APMT equ 1 << 10
VIDEO_FRAMEBUF equ 1 << 12

MULTIBOOT_FLAGS equ MEMORY_INFO | BOOT_DEV | CMDLINE | MODS | SYMBOL_TABLE | MEMORY_MAP | DRIVE | CONFIG_TABLE | BOOTLOADER_NAME | APMT | VIDEO_FRAMEBUF
MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

KERNEL_MAGIC equ 0x43D8C305 ; reduceOS

section .multiboot 
    align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

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