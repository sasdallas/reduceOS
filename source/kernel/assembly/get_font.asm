[bits 32]
[global get_font]

get_font:
    mov eax, edi
    
    ; Clear even/odd mode
    mov dx, 03ceh
    mov ax, 5
    out dx, ax

    ; Map the VGA memory to 0A0000h
    mov ax, 0406h
    mov dx, ax

    ; Set bitplane 2
    mov dx, 03c4h
    mov ax, 0402h
    out dx, ax

    ; Clear even/odd mode the other way
    mov ax, 0604h
    out dx, ax

    ; Copy charmap
    mov esi, 0A0000h
    mov ecx, 256

    ; Copy 16 bytes to bitmap
.loop movsd
    movsd
    movsd
    movsd
    ; Skip another 16 bytes
    add esi, 16
    loop .loop

    ; Restore VGA to normal operation
    mov ax, 0302h
    out dx, ax
    mov ax, 0204h
    out dx, ax
    mov dx, 03ceh
    mov ax, 1005h
    out dx, ax
    mov ax, 0E06h
    out dx, ax 
    mov edi, eax
    ret