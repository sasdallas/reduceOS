# reduceOS multiboot loader - written by sasdallas.
# Compatible with multiboot 1 specifications (multiboot 2 is todo)


# External variables
.extern  bss_start
.extern bss_end
.extern start

# Multiboot 1 header.
# Magic: 0x1BADB002

.set MULTIBOOT_MAGIC, 0x1BADB002
.set ALIGN, 1 << 0
.set MEMORY_INFO, 1 << 1
.set MODS, 1 << 3


.set MULTIBOOT_FLAGS, ALIGN | MEMORY_INFO | MODS
.set CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

# Multiboot 1 section
.section .multiboot
.align 4
.long MULTIBOOT_MAGIC # Multiboot magic
.long MULTIBOOT_FLAGS # Multiboot flags
.long CHECKSUM # Checksum


# Our stack resides in BSS
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KB of stack to start off with
stack_top:


# The kernel entry point.
.section .text
.global _start
.global stack_bottom

.type _start, @function
_start:
    movl $stack_top, %esp
    call _init

    xor %ebp, %ebp
    push %ebx
    call kmain

    # System hung, stop CPU and halt in infinite loop.
    cli
1:  hlt
    jmp 1b

.size _start, . - _start


