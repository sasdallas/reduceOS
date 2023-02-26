; -----------------------------------
; a20.inc - Enabling A20 Line
; -----------------------------------


; First, show that we are defining a20.inc
%ifndef __A20_INC_67343546FDCC56AAB872_INCLUDED__
%define __A20_INC_67343546FDCC56AAB872_INCLUDED__

bits 16 ; We are still 16 bits when we enable A20


; -------------------------------------------------------------
; enableA20_KKbrd - enabling A20 through keyboard controller
; -------------------------------------------------------------

enableA20_KKbrd:
    cli                     ; Clear interrupts
    push ax                 ; Save AX
    mov al, 0xdd            ; A20 address line command
    out 0x64, al            ; Send A20 address line command to controller
    pop ax                  ; Restore AX
    ret

; ----------------------------------------------------------
; enableA20_KKbrd_Out - Enabling A20 through output port
; ----------------------------------------------------------

enableA20_KKbrd_Out:
    cli                     ; Clear interrupts
    pusha
    
    call waitInput          
    mov al, 0xAD            ; Disable keyboard command
    out 0x64, al            ; Disable the keyboard
    call waitInput          

    mov al, 0xD0            ; Read output port command
    out 0x64, al            ; Send read output port command to controller
    call waitOutput         ; Wait for output

    in al, 0x60              
    push eax                ; Get the output port data and store it
    call waitInput
    
    mov al, 0xD1            ; Write output port command
    out 0x64, al            ; Send write output port command to controller
    call waitInput          

    pop eax
    or al, 2                ; Set bit 1(enable A20)
    out 0x60, al            ; Write data back to the output port

    call waitInput          
    mov al, 0xAE            ; Enable keyboard command
    out 0x64, al            ; Enable keyboard

    call waitInput          

    popa
    sti                     ; Enable interrupts
    ret                     ; Return

; ---------------------------------------------------
; waitInput - waits for input buffer to be cleared
; ---------------------------------------------------

waitInput:
    in al, 0x64
    test al, 2
    jnz waitInput
    ret


; ----------------------------------------------------
; waitOutput - waits for input buffer to be cleared
; ----------------------------------------------------

waitOutput:
    in al, 0x64
    test al, 1
    jz waitOutput
    ret


; ---------------------------------------------
; enableA20_BIOS - Enables A20 through BIOS
; ---------------------------------------------

enableA20_BIOS:
    pusha                   ; Restore Registers
    mov ax, 0x2401
    int 0x15                ; Call BIOS
    popa                    ; Restore registers
    ret                     ; Return

; ---------------------------------------------------------------------
; enableA20_sysControlA - Enable A20 through system control port A
; ---------------------------------------------------------------------

enableA20_sysControlA:
    push ax                 ; Save AX
    mov al, 2
    out 0x92, al            ; Enable A20
    pop ax                  ; Restore AX
    ret

%endif
