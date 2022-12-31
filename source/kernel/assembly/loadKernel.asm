; reduceOS multiboot loader - written by sasdallas

[BITS 32]
[GLOBAL multiboot]
[EXTERN text]
[EXTERN bss]
[EXTERN end]


MULTIBOOT_PAGE_ALIGN equ 1 << 0
MEMORY_INFO equ 1 << 1
BOOTDEVICE equ 1<<1
CMDLINE equ 1<<2
MODULECOUNT equ 1<<3
SYMT equ 48
MEMMAP equ 1<<6
DRIVE equ 1<<7
CONFIGT equ 1<<8
BOOTLDNAME equ 1<<9
APMT equ 1<<10
VIDEO equ 1<<11
VIDEO_FRAMEBUF equ 1<<12

MULTIBOOT_FLAGS equ MULTIBOOT_PAGE_ALIGN | MEMORY_INFO | BOOTDEVICE | CMDLINE | MODULECOUNT | SYMT | MEMMAP | DRIVE | CONFIGT | BOOTLDNAME | APMT | VIDEO | VIDEO_FRAMEBUF
MULTIBOOT_HEADER_MAGIC equ 0x1BADB002
MULTIBOOT_MAGIC equ 0x2BADB002
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_FLAGS)


multiboot:
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

    dd multiboot
    dd text
    dd bss
    dd end
    dd _start




[GLOBAL _start]
[EXTERN kmain]

_start:
    push ebx
    call kmain
    jmp $