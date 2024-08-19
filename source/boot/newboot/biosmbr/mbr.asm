; ================================================================
; mbr.asm - El-Torito (ISO9660) style master boot record
; ================================================================
; This file is part of the Polyaniline bootloader. Please credit me if you use this code.
; THIS FILE IS NOT CURRENTLY BEING USED.


org 0x7c00                                  ; The classic MBR start address
bits 16                                     ; Back to 16-bit we go!

jmp main                                    ; Not technically required

; This is basically a modded ISO9660 bootloader, we're (sort of) just hacking it together.
; A buildscript is run to automatically modify the below lines before compilation, and during installation, the lines are reset.
; The lines are reset to prevent any problems with them, as we're not using GAS style assembly code.
; Man, I wish we had --defsym...

BootFileOffset dw 0x0
BootFileSize dw 0x0

print:
    lodsb
    or al, al
    jz donePrint
    mov bx, 0007h
    mov ah, 0Eh
    int 10h
    jmp print
donePrint:
    ret


PrintError:
    mov si, Str_BootloaderConfigurationInvaild
    call print 
    cli
    hlt    

; main - MBR function where the magic happens
main:
    ; The bootloader will just crash if this breaks, so no printing is necessary (besides invalid configuration)..
    ; It's gonna be pretty hacked together

    ; Let's setup our segment registers (by zeroing them out) and a stack at 0x8000, just like the legacy MBR.
    cli
    xor  ax, ax
    mov  ds, ax
    mov  [BootDisk], dl                       ; We do want to save DL before everything gets too chaotic
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    mov  ax, 0x8000    ; Stack between 0x8000:0x0000
    mov  ss, ax        ;           and 0x8000:0xFFFF (64KB)
    xor  sp, sp
    sti

    ; Perform the check before anything stupid
    push ax                                 ; Save to stack
    mov ax, [BootFileOffset]                ; Is boot file offset 0?
    cmp ax, 0
    je PrintError                           ; Yes - halt & print error

    mov ax, [BootFileSize]                  ; Is boot file size 0?
    cmp ax, 0
    je PrintError                           ; Yes - halt & print error
    pop ax



    ; AH=0x48 DL=BootDisk SI=DriveParams - Get drive parameters
    mov ah, 0x48
    mov dl, [BootDisk]
    mov si, [DriveParams]
    int 0x13                                ; Call the BIOS

    ; We need to calculate the first sector of boot.sys (LBA)
    mov edx, 0x0                            ; Terrifying! 32-bit variables in 16-bit mode!
    mov eax, [BootFileOffset]
    mov ecx, [DriveParamsBytesPerSector]
    div ecx
    mov [File_LbaLow], eax
    mov eax, [BootFileSize]
    div ecx
    inc eax
    mov ax, [File_Sectors]
    mov dword [FileBuffer], 0x7E00

    ; AH=0x42 DL=BootDisk SI=DAP (Disk Address Packet, right?) - Extended read
    mov ah, 0x42
    mov dl, BootDisk
    mov si, DAP
    int 0x13

    xor ax, ax
    mov ax, es
    mov ax, ds

    ; Now let's do an MBR move with a loop, then jump to our code
    mov esi, moveLoop
    mov edi, 0x7B00                         ; Move location
    mov ecx, [moveFail-moveLoop]
    rep movsb
    mov eax, 0x7B00                         ; Jump location
    jmp eax                                 ; Done!

moveLoop:
    mov esi, 0x7E00
    mov edi, 0x7C00
    mov ecx, BootFileSize
    rep movsb
    mov eax, 0x7C00
    jmp eax
moveFail:
    ; Let's go out in style. Far jump to reset vector
    jmp 0xFFFF:0



BootDisk db 0x0

DriveParams:
    dw 0x1A
    dw 0            ; Flags
    dw 0            ; Cylinders
    dw 0            ; Heads
    dd 0            ; Sectors
    dd 0            ; Total sectors
    dd 0

DriveParamsBytesPerSector dw 0

; Buffers
File_LbaLow dd 0x0
File_LbaHigh: dd 0x0
File_Sectors dw 0x0


FileBuffer dd 0x0

DAP:
    db 16
    db 0


; One string though
Str_BootloaderConfigurationInvaild db "The bootloader is improperly configured. Boot file parameters are invalid.", 13, 10, 0



times 510 - ($-$$) db 0                     ; Padding for 510 bytes
dw 0xAA55                                   ; Bootable signature