; ===============================================
; 32-bit kernel for reduceOS
; ===============================================
; Loaded in by loader.asm at 0x0600,
; With <3 and thanks to the BrokenThorn Entertainment and StackOverflow

bits 16                                 ; Beginning at 16-bits, switching to 32-bit later            
org  0x07C0                             ; Starting at address 0x0600, loaded in by loader.asm


jmp  main
; ---------------------------------------
; Includes
; --------------------------------------

%include "include/stdio.inc"
%include "include/gdt.inc"
%include "include/a20.inc"
%include "include/memory.inc"
%include "include/bootinfo.inc"

; ---------------------------------------
; Data and strings
; ---------------------------------------


preparing db "Loading reduceOS...", 0x0D, 0x0A, 0x00
readErrorStr db "Error reading sectors.", 0x0D, 0x0A, 0x00
kernelOffset equ 0x1100

; Credit to BrokenThorn Entertainment for their structure of multiboot.
boot_info:
istruc multiboot_info
	at multiboot_info.flags,			dd 0
	at multiboot_info.memoryLo,			dd 0
	at multiboot_info.memoryHi,			dd 0
	at multiboot_info.bootDevice,			dd 0
	at multiboot_info.cmdLine,			dd 0
	at multiboot_info.mods_count,			dd 0
	at multiboot_info.mods_addr,			dd 0
	at multiboot_info.syms0,			dd 0
	at multiboot_info.syms1,			dd 0
	at multiboot_info.syms2,			dd 0
	at multiboot_info.mmap_length,			dd 0
	at multiboot_info.mmap_addr,			dd 0
	at multiboot_info.drives_length,		dd 0
	at multiboot_info.drives_addr,			dd 0
	at multiboot_info.config_table,			dd 0
	at multiboot_info.bootloader_name,		dd 0
	at multiboot_info.apm_table,			dd 0
	at multiboot_info.vbe_control_info,		dd 0
	at multiboot_info.vbe_mode_info,		dw 0
	at multiboot_info.vbe_interface_seg,		dw 0
	at multiboot_info.vbe_interface_off,		dw 0
	at multiboot_info.vbe_interface_len,		dw 0
iend


; -------------------------------------------------------------------
; readSector - Purpose is to load the actual C kernel to 0x1000
; Parameters: CL, ES, BX, AL
; -------------------------------------------------------------------

readSector:
    mov ah, 2h                  
    mov dh, 0
    mov ch, 0
    int 13h
    jnc loadGood
    
fail:  
    mov si, readErrorStr
    call print16
    int 0x16
stop:
    cli
    hlt
    jmp stop

loadGood:
    mov si, preparing
    call print16
    ret




; ---------------------------------------
; main - 16-bit entry point
; Installing GDT, storing BIOS info, and enabling protected mode
; ---------------------------------------

main:
    ; The loader has successfully loaded us to 0x0600! Continue with execution to protected mode!
    
    call installGDT                             ; Install the GDT
    call enableA20_KKbrd_Out                    ; Enable A20
    
    mov si, preparing                           ; Print our preparing string
    call print16

    
    ; Before we enter protected mode, we need to load our C kernel. 
    ; BIOS interrupts aren't supported in pmode, so we do it here. 
    ; ES is already set to the proper values.
    mov al, 40                                  ; AL - sector amount to read
    mov bx, kernelOffset                        ; Read to 0x1000 (remember it reads to ES:BX)
    mov cl, 5                                   ; Starting from sector 4, which in our case is 0x600(where the code is located)
    call readSector                             ; Read the sector
    
    xor		eax, eax
	xor		ebx, ebx
	call biosGetMemorySize64MB                  ; Get memory

	push eax
	mov	eax, 64
	mul ebx
	mov ecx, eax
	pop eax
	add eax, ecx
	add	eax, 1024		                        ; biosGetMemorySize64MB doesn't add KB between 0 to 1 MB, so we add it ourselves.

	mov dword [boot_info+multiboot_info.memoryHi], 0    ; Update memoryHi and memoryLo to their proper values.
	mov	dword [boot_info+multiboot_info.memoryLo], eax

	mov eax, 0x0
	mov	ds, ax
	mov	di, 0x1000
	call biosGetMemoryMap                       ; Get the memory map.

    mov esi, kernelOffset
    call GetSize

    ; Now we need to enable protected mode.

EnablePmode:

    ; Enter protected mode
    cli
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    jmp  CODE_DESC:main32                       ; Far jump to fix CS



bits 32                                         ; We are now 32 bit!

%include "include/stdio32.inc"                  ; Printing
%include "include/paging.inc"                   ; Paging


main32:
    ; Set registers up
    mov  ax, DATA_DESC
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax                                 ; Stack between 0x00000000
    mov  esp, 0x00090000                        ;           and 0x0008FFFF (576KB)

    call clear32                                ; Clear screen
    mov  ebx, buildNum
    call puts32                                 ; Call puts32 to print

    mov ebx, kernOK                             ; Print our kernel okay message
    call puts32           

    call enablePaging                           ; Enable paging.

    mov eax, boot_info                          ; Push boot_info for the kernel.
    push eax
    call kernelOffset                           ; The kernel is located at 
    jmp $

    

buildNum db 0x0A, 0x0A, 0x0A, "                         reduceOS Development Build", 0
kernOK db 0x0A, 0x0A, 0x0A, "                              Loading C kernel...", 0
