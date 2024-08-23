; ----------------------------------------------------------
; panic16.inc - handles terminating the assembly kernel
; Only used in 16-bit mode. No 32-bit panic handler.
; ----------------------------------------------------------

BITS 16                                     ; We are only 16-bits


; ===================================================
; panic - function that terminates loading OS.
; Parameters - SI (string to use)
; ===================================================

panic:
    mov ax, si                              ; Temporarily store SI in AX.
    
    mov si, ERROR_STR1                      ; Print all error strings
    call print16
    mov si, ERROR_STR2
    call print16

    mov si, ax                              ; Now print the error.
    call print16

    cli                                     ; Halt system.
    hlt


ERROR_STR1 db "A fatal error occurred while attempting to load reduceOS. The system has been terminated.", 0x0D, 0x0A, 0x00
ERROR_STR2 db "The error was:", 0x0D, 0x0A, 0x00
