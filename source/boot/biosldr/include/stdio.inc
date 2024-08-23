; ==============================================
; stdio.inc - IO routines
; ==============================================

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
    mov ah, 0eh                                 ; 0Eh = teletype output
    int 10h                                     ; Call BIOS
    jmp .loop1                                  ; Repeat!
print16done:
    popa                                        ; Restore registers
    ret                                         ; Return


