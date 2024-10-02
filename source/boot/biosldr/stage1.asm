; ===========================================================
; stage1.asm - Startpoint for Polyaniline (after MBR)
; ===========================================================
; This file is part of the Polyaniline bootloader. Please credit me if you use this code.

bits 16

jmp main

; ---------------------------------------
; Includes
; ---------------------------------------

%include "include/stdio.inc"
%include "include/gdt.inc"
%include "include/a20.inc"
%include "include/memory.inc"
%include "include/bootinfo.inc"




; ---------------------------------------
; Data and strings
; ---------------------------------------
preparing db "Preparing to load Polyaniline, please wait...", 13, 10, 0
BootDisk db 0x0



; ---------------------------------------
; Functions
; ---------------------------------------
main:
	mov si, preparing
	call print16
	cli
	hlt
	

	; Now we'll enable protected mode
enablePmode:
	cli
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	jmp CODE_DESC:main32


; ---------------------------------------------------------------------------------

bits 32											; Welcome to the wide world of 32-bits

%include "include/stdio32.inc"
%include "include/paging.inc"

main32:
	cli
	hlt

	; Setup registers
	mov ax, DATA_DESC
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0x00090000

	call clear32
	mov ebx, kernOK
	call puts32
	
.infloop:
	cli
	hlt
	jmp .infloop


; 32-bit mode data and strings
buildNum db 0x0A, 0x0A, 0x0A, "                         reduceOS Development Build", 0
kernOK db 0x0A, 0x0A, 0x0A, "                              Loading C kernel...", 0