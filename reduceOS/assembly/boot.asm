MBALIGN		equ 1<<0 ; align loaded modules on this
MEMINFO		equ 1<<1 ; memory map
FLAGS 		equ MBALIGN | MEMINFO ; multiboot flags
MAGIC		equ 0x1BADB002 ; magic multiboot number
CHECKSUM	equ -(MAGIC + FLAGS) ; checksum to verify

; Multiboot section
section .multiboot
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

section .data
	align 4096


; initial stack
section .initial_stack, nobits
	align 4

stack_bottom:
	; 1MB of uninitialized
	resb 104856

stack_top:


section .text
	global _start

_start:
	mov esp, stack_top
	extern kernel_main
	push ebx
	call kernel_main
loop:
	jmp loop