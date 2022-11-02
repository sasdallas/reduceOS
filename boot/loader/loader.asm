; A note to all viewing this file:
; This file is subject to recommentation and cleanup later, sorry for the mess!
bits 16
org  0x7C00

jmp  main

; args: SI
print:
    lodsb
    or   al, al
    jz   donePrint
    mov  bx, 0007h
    mov  ah, 0Eh
    int  10h
    jmp  print
donePrint:
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

readSector:
    mov  dh, 0
    mov  cx, 0002h
    mov  bx, 0x0600     ; Sector buffer at 0x0000:0x0600
    mov  ax, 0201h
    int  13h
    jnc  good
    
fail:
    mov  si, FAILURE_MSG
    call print
end:
    cli
    hlt
    jmp  end
    
good:
    mov  si, LOADOK 
    call print
    jmp  0x0000:0x0600  ; Start second stage

LOADING     db 13, 10, "Booting loader...", 13, 10, 0
FAILURE_MSG db 13, 10, "ERROR: Press any key to reboot.", 10, 0
LOADOK      db 13, 10, "load ok", 10, 0

TIMES 510 - ($-$$) DB 0
DW 0xAA55
