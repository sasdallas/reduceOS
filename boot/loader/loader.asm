; ==================================================
; loader.asm - reduceOS custom bootloader
; ==================================================


bits 16
org  0x7C00

jmp  main

; args: SI
print:
    lodsb                       ; Load character
    or   al, al                 ; Is the character 0?
    jz   donePrint              ; If so, return.
    mov  bx, 0007h              ; If not, continue printing.
    mov  ah, 0Eh                ; 0Eh = teletype output
    int  10h                    ; Call BIOS
    jmp  print                  ; We know we're not done printing yet, so repeat the function.
donePrint:
    ret



; =====================================================
; readSector - reads a sector from disk
; Jumps to loadGood if sector OK, halts if not.
; Parameters: CX, BX, AX
; =====================================================
readSector:
    mov ah, 2h                  ; AH = second function of int 13h
    mov al, 2                    ; Reading 2 sectors.
    mov ch, 0                   ; Cylinder 0
    mov cl, 2                   ; Starting from sector 2(begins from 1 not 0)
    mov dh, 0                   ; Drive #0
    mov bx, 0x0600              ; Read to 0x0000:0x0600
    int  13h                    ; Read!
    jnc loadGood                ; Return if it's good.

fail:
    mov  si, FAILURE_MSG
    call print
stop:
    cli
    hlt
    jmp  stop


loadGood:
    mov  si, LOADOK 
    call print
    ret

main:
    cli
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ax, 0x8000    ; Stack between 0x8000:0x0000
    mov  ss, ax        ;           and 0x8000:0xFFFF (64KB)
    xor  sp, sp
    sti

    mov  si, LOADING
    call print


    call readSector
    jmp  0x0000:0x0600  ; Jump to second stage

    

    

    
LOADING     db 13, 10, "Booting loader...", 13, 10, 0
FAILURE_MSG db 13, 10, "ERROR: Press any key to reboot.", 10, 0
LOADOK      db "Sector load OK."

TIMES 510 - ($-$$) DB 0
DW 0xAA55
