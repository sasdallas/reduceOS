; ==============================================
; stdio.inc - IO routines
; Thanks to BrokenThorn Entertainment
; ==============================================

; First, show that we are defining stdio.inc
%ifndef __STDIO_INC_67343546FDCC56AAB872_INCLUDED__
%define __STDIO_INC_67343546FDCC56AAB872_INCLUDED__

; ------------------------------------------------
; Print16 - printing a null terminated string
; SI - 0 terminated string
; ------------------------------------------------

print16:
    pusha                                       ; Save registers for later
.loop1:
    lodsb                                       ; Load the next byte from the string into AL
    or al, al                                   ; Is AL 0?
    jz print16done                              ; Yes - we are done.
    mov ah, 0eh                                 ; No - print next character
    int 10h                                     ; Call BIOS
    jmp .loop1                                  ; Repeat!
print16done:
    popa                                        ; Restore registers
    ret                                         ; Return



%endif ;__STDIO_INC_67343546FDCC56AAB872_INCLUDED__
