; ==================================================
; gdt.inc - Loading the Global Descriptor Table
; ==================================================

; First, we show we are defining gdt.inc
%ifndef __GDT_INC_67343546FDCC56AAB872_INCLUDED__
%define __GDT_INC_67343546FDCC56AAB872_INCLUDED__

bits 16                     ; We are in 16-bit mode


; -----------------------------------------
; installGDT - install the GDT
; -----------------------------------------
installGDT:
    cli                     ; Clear interrupts
    pusha                   ; Save the registers
    lgdt [toc]              ; Load GDT into GDTR
    sti                     ; Re-enable interrupts
    popa                    ; Restore registers
    ret                     ; Return!


; ----------------------------------------
; Global Descriptor Table data
; ----------------------------------------

gdt_data:
    dd 0                    ; Null descriptor
    dd 0
    
    ; GDT code starts here
    dw 0FFFFh               ; Limit low
    dw 0                    ; Base low
    db 0                    ; Base middle
    db 10011010b            ; Access
    db 11001111b            ; Granularity
    db 0                    ; Base high

    ; GDT data starts here(mostly same as code, only difference is access)
    dw 0FFFFh               ; Limit low, again.
    dw 0                    ; Base low
    db 0                    ; Base middle
    db 10010010b            ; Access - different
    db 11001111b            ; Granularity
    db 0

gdt_end:
toc:
    dw gdt_end - gdt_data - 1
    dd gdt_data             ; Base of GDT

; Descriptor offsets names

%define NULL_DESC 0
%define CODE_DESC 0x8
%define DATA_DESC 0x10

; End of GDT code.
%endif ;__GDT_INC_67343546FDCC56AAB872_INCLUDED__