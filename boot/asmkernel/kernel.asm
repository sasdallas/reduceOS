; ===============================================
; 32-bit kernel for reduceOS
; ===============================================
; Loaded in by loader.asm at 0x500,
; With <3 and thanks to the BrokenThorn Entertainment

bits 16                                 ; Beginning at 16-bits, switching to 32-bit later            
org  0x0600                             ; Starting at address 0x0600, loaded in by loader.asm


jmp  main
; ---------------------------------------
; Includes
; ---------------------------------------

%include "include/stdio.inc"
%include "include/gdt.inc"
%include "include/a20.inc"

; ---------------------------------------
; Data and strings
; ---------------------------------------

kernel_loaded db "Kernel loaded successfully.", 0x0D, 0x0A, 0x00
preparing db "Preparing to load reduceOS...", 0x0D, 0x0A, 0x00

; ---------------------------------------
; main - 16-bit entry point
; Installing GDT, storing BIOS info, and enabling protected mode
; ---------------------------------------

main:
    call installGDT
    call enableA20_KKbrd_Out
    mov  si, kernel_loaded
    call print16
    mov  si, preparing
    call print16

    ; Enter protected mode
    cli
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    jmp  CODE_DESC:main32 ; Far jump to fix CS
    


bits 32                   ; We are now 32 bit!

%include "include/stdio32.inc"

main32:
    ; Set registers up
    mov  ax, DATA_DESC
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ss, ax           ; Stack between 0x00000000
    mov  esp, 0x00090000  ;           and 0x0008FFFF (576KB)

    call clear32          ; Clear screen
    mov  ebx, buildNum
    call puts32           ; Call puts32 to print

    cli
    hlt

buildNum db 0x0A, 0x0A, 0x0A, "reduceOS test build", 0
