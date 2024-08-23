; ======================================================
; stdio32.inc - Handles 32-bit graphics for reduceOS
; ======================================================
; Thanks to BrokenThorn Entertainment and Sep Roland!
%ifndef __GFX_INC_67343546FDCC56AAB872_INCLUDED__
%define __GFX_INC_67343546FDCC56AAB872_INCLUDED__

bits 32

%define VIDEO_MEMORY 0xB8000  ; Video memory address
%define COLS 80               ; Width of the screen
%define LINES 25              ; Height of the screen
%define ATTRIB 0x3F           ; WhiteOnCyan
_CurrentXPos db 0
_CurrentYPos db 0

; ---------------------------------------------------------
; char32 - Print a character to the screen(32-bit)
;   BL - Character to print
; ---------------------------------------------------------

char32:
    cmp   bl, 10
    je    .row
    push  eax
    push  edi
    movzx edi, byte [_CurrentYPos]
    imul  edi, COLS*2
    movzx eax, byte [_CurrentXPos]
    lea   edi, [VIDEO_MEMORY + edi + eax * 2]
    mov   al, bl
    mov   ah, ATTRIB
    mov   [edi], ax
    pop   edi
    pop   eax
    inc   byte [_CurrentXPos]
    cmp   byte [_CurrentXPos], COLS               ; EOL?
    je    .row
    ret
.row:
    mov   byte [_CurrentXPos], 0
    inc   byte [_CurrentYPos]
    ret

; ---------------------------------------------------------
; puts32 - print a null terminated string
;   EBX - String to print
; ---------------------------------------------------------

puts32:

    push  ebx
    push  edi
    mov   edi, ebx
    jmp   .start
.loop:
    call  char32
    inc   edi
.start:
    mov   bl, [edi]
    test  bl, bl
    jnz   .loop
    movzx ebx, word [_CurrentXPos] ; Load XPos and YPos together!
    call  movecursor               ; Update hardware cursor
    pop   edi
    pop   ebx
    ret

; ---------------------------------------------------------
; movecursor - Move the cursor to an X and Y position
;   BL - X position
;   BH - Y position
;   BH and BL are relative to the current position on screen
; ---------------------------------------------------------

movecursor:
    pushad
    movzx eax, bh             ; BH * COLS + BL
    imul  eax, COLS
    movzx ebx, bl
    add   ebx, eax
    ; Set low byte index to VGA register
    mov   al, 0x0F
    mov   dx, 0x03D4
    out   dx, al
    mov   al, bl
    inc   dx
    out   dx, al
    ; Do the same but for high byte
    mov   al, 0x0E
    dec   dx
    out   dx, al
    mov   al, bh
    inc   dx
    out   dx, al
    popad
    ret

; ---------------------------------------------------------
; clear32 - clearing the screen
; ---------------------------------------------------------

clear32:
    pushad
    mov   edi, VIDEO_MEMORY
    mov   ecx, 1000            ; 2000 words
    mov   eax, ((ATTRIB * 256 + 32) * 256 + ATTRIB) * 256 + 32
    rep stosd
    mov   [_CurrentXPos], cx   ; Reset XPos and YPos together!
    popad
    ret

; ---------------------------------------------------------
; gotoxy - Set X and Y position
;   AL - X position
;   AH - Y position
; ---------------------------------------------------------

gotoxy:
    mov   [_CurrentXPos], ax   ; Set XPos and YPos together!
    ret

%endif ;__STDIO_INC_67343546FDCC56AAB872_INCLUDED__
