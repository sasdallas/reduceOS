; ==================================================
; stdio32.inc - Handles 32-bit graphics
; ==================================================

%ifndef __GFX_INC_67343546FDCC56AAB872_INCLUDED__
%define __GFX_INC_67343546FDCC56AAB872_INCLUDED__

bits 32                                         ; 32-bits

%define VIDEO_MEMORY 0xB8000                    ; Video memory address
%define COLS 80                                 ; Width of the screen
%define LINES 25                                ; Height of the string
%define CHARACTER_ATTRIBURE 63                  ; White text on Cyan background

_CurrentXPos db 0
_CurrentYPos db 0

; ---------------------------------------------------------
; char32 - Print a character to the screen(32-bit)
;   BL - Character to print
; ---------------------------------------------------------

char32:
    pusha                                       ; Save registers
    mov edi, VIDEO_MEMORY                       ; Get the pointer to the video memory

    ; Get current position
    xor eax, eax                                ; Zero-out EAX

    mov ecx, COLS*2                             ; Mode 7 has 2 bytes per character - and so COLS*2 bytes per line.
    mov al, byte [_CurrentYPos]                 ; Get Y position
    mul ecx                                     ; Multiply COLS * Y
    push eax                                    ; Save EAX--the multiplication

    mov al, byte [_CurrentXPos]                 ; Multiply _CurrentXPos by 2 because 2 bytes per char(Mode 7)
    mov cl, 2
    mul cl
    pop ecx                                     ; Pop Y*COLS result
    add eax, ecx
    
    xor ecx, ecx
    add edi, eax                                ; Add to base address

    ; Watch for a new line!
    cmp bl, 0x0A                                ; 0x0A - newline character.
    je .row                                     ; Jump to .row if newline char

    ; Print the character
    mov dl, bl                                  ; Get character
    mov dh, CHARACTER_ATTRIBURE                 ; Change DH to Character Attribute
    mov word [edi], dx                          ; Write to video memory
    
    ; Update next pos
    inc byte [_CurrentXPos]                     ; Go to next character
    ;cmp byte [_CurrentXPos], COLS               ; EOL?
    ;je .row                                     ; Yep - move to next row
    jmp .done                                   ; Nope - BAIL!
.row:
    ; Goto next row.
    mov byte [_CurrentXPos], 0                  ; Return to col 0
    inc byte [_CurrentYPos]                     ; Go to next row.

.done:
    ; Return
    popa
    ret


; ---------------------------------------------------------
; puts32 - print a null terminated string
;   EBX - String to print
; ---------------------------------------------------------

puts32:

    ; Store registers(EBX and EDI)
    pusha                                       ; Save registers
    push ebx                                    ; Copy string
    pop edi

.loop:
    
    mov bl, byte [edi]                          ; Get next character
    cmp bl, 0                                   ; Check if it's null
    je .done                                    ; It is - done printing.

    call char32                                 ; It isn't - print the character

    inc edi                                     ; Increment EDI for next character
    jmp .loop                                   ; Restart loop

.done:
    ; Update the hardware cursor

    mov bh, byte [_CurrentXPos]                 ; BH and BL are the params for movecursor
    mov bl, byte [_CurrentYPos]                 
    call movecursor                             ; Update cursor position

    popa                                        ; Restore registers
    ret                                         ; Return!

bits 32

; ---------------------------------------------------------
; movecursor - Move the cursor to an X and Y position
;   BH - X position
;   BL - Y position
; ---------------------------------------------------------

movecursor:
    
    pusha                                       ; Save registers

    ; Get current position(BH and BL are relative to the current position on screen, not memory)

    xor eax, eax                                ; Clear EAX
    mov ecx, COLS                               ; Store COLS in ECX for multiplication
    mov al, bh                                  ; Get Y position
    mul ecx                                     ; Multiply Y by cols
    add al, bl                                  ; Add X
    mov ebx, eax

    ; Set low byte index to VGA register

    mov al, 0x0f
    mov dx, 0x03D4
    out dx, al

    mov al, bl
    mov dx, 0x03D5
    out dx, al

    ; Do the same but for high byte

    xor eax, eax
    
    mov al, 0x0e
    mov dx, 0x03D4
    out dx, al

    mov al, bl
    mov dx, 0x03D5
    out dx, al

    popa                                        ; Restore registers
    ret                                         ; Return



; ---------------------------------------------------------
; clear32 - clearing the screen
; ---------------------------------------------------------

clear32:
    pusha                                       ; Save registers

    cld
    mov edi, VIDEO_MEMORY                       ; Set EDI to video memory
    mov cx, 2000                                
    mov ah, CHARACTER_ATTRIBURE                 ; Clear screen with character attribute
    mov al, ' '                                 ; Replace all chars with space
    rep stosw

    mov byte [_CurrentXPos], 0                  ; Reset X and Y position
    mov byte [_CurrentYPos], 0
    popa                                        ; Restore registers
    ret

    

; ---------------------------------------------------------
; gotoxy - Set X and Y position
;   AL - X position
;   AH - Y position
; ---------------------------------------------------------

gotoxy:
    pusha
    mov [_CurrentXPos], al                      ; Set X and Y position
    mov [_CurrentYPos], ah                      
    popa
    ret

%endif ;__STDIO_INC_67343546FDCC56AAB872_INCLUDED__