; =======================================================
; start.asm - Startpoint for reduced_loader (after MBR)
; =======================================================

bits 16
org 0x0
jmp main

print:
.loop:
  lodsb                                 ; Load character from SI buffer
  or al, al                             ; Is it a zero?
  jz .print_done                        ; Done printing :D 
  mov ah, 0eh                           ; BIOS system call
  int 10h                               ; Call BIOS
  jmp .loop                             ; Loop!
.print_done:
  ret                                   ; Return  



main:
  cli                                   ; Clear interrupts
  push cs
  pop ds

  ; Print loading message
  mov si, loading_msg
  call print

  cli                                   ; Clear interrupts
  hlt                                   ; Halt system


loading_msg db "reduced_loader is starting...", 13, 10, 0