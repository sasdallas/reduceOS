; mbr.asm - reduceOS MBR loader
; This file is part of the reduceOS bootloader. Please credit me if you use this code.

; DESCRIPTION: Loads a DOS FAT32 partition and does a VBR handoff.
; TODO: GPT-style partition tables
; BUGS: Probably the whole code ;)
; CREDITS: Take a look at the boot-pcbios project, it provides a FAT32 VBR and MBR, which is what this impl. is based off.


[BITS 16]
[ORG 0x0]

; Compatibility shenanigans where we fix stack pointers and other things
fakestart:
    cli
    call realstart

realstart:
    ; Copy us to 0x7c00.
    pop si                                      
    lea si, [si-(realstart-fakestart)]   ; SI = start address
    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00                      ; This is where we actually want to land
    mov al, 0x60
    cld                                 ; CLD, increment pointer
    mov es, ax
    xor di, di                          ; Loaded at 0x0 (copy from DI->SI)
    mov cx, 0x100                       ; Repeat 0x100 times
    rep cs movsw
    sti                                 ; Enable interrupts
    mov cl, main - fakestart
    push es
    push cx
    retf

; main - The actual code
main:
    push cs
    pop ds
    ; Setup parameters for reading
    mov si, partitions_layout
    mov cl, 4
.loop:
    test byte [si], 0x80                ; Are we ready to load?
    jnz load                            ; Yes, load.
    add si, 16                          ; Increment and try again
    loop .loop
    mov si, msg_error                   ; We could not do it
    call printerror

; Load a new sector
load:
    mov ax, 0x07c0
    mov es, ax
    mov bx, dx
    mov ax, [si+0x8]
    mov dx, [si+0xA]
    call read_sector
    mov si, msg_notfound                ; Prematurely load the message
    cmp word [es:0x1FE], 0xAA55         ; Check for signature
    jne printerror                      ; No signature - couldn't find
    mov dx, bx                          ; We did find it - let's jump to it!
    jmp 0x0000:0x7c00


disk_error:
    mov si, msg_diskerror
    call printerror

; read_sector - Actually reading the sector using INT 13
; Parameters: DX:AX (sector to read), ES:0x0000 (destination), BL (drive number)
read_sector:
    push bp
    push ds
    push di
    push si
    push ax 
    push cx
    push dx
    mov cx, 3                           ; IMPORTANT: This is the maximum amount of tries
    push bx
.retry:
    push cx
    mov bp, sp
    xor cx, cx
    
    ; Push the LBA values
    push cx                             ; bp-2 - LBA [63 .. 48]
    push cx                             ; bp-4 - LBA [47 .. 32]
    push dx                             ; bp-6 - LBA [31 .. 16]
    push ax                             ; bp-8 - LBA [15 .. 0]
    push es                             ; bp-10 - Destination Segment
    push cx                             ; bp-12 - Destination offset
    inc cx
    push cx                             ; bp-14 - Blocks to tarnsfer
    mov cl, 16 
    push cx                             ; bp-16 - Size of Packet
    mov si, sp 
    mov ah, 0x08
    mov dl, [bp+2]
    int 0x13                            ; Read the disk
    jc disk_error                       ; Disk error
    mov ax, 0x3f
    and cx, ax                          ; CX = SPT
    mov al, dh
    inc ax                              ; AX = HPC
    mul cx
    xchg bx, ax                         ; BX = HPC*SPT
    mov dx, [bp-6]
    cmp dx, bx                          ; Will the division overflow?
    jae .lba                            ; Yes, use LBA
    mov ax, [bp-8]
    div bx                              ; DX = Cylinder
    xchg ax, dx                         ; AL = Head
    div cl                              ; AH = Sector - 1
    mov cl, 2
    xchg ch, dl                         ; DL == zero; ch = low bits of cylinder
    shr dx, cl                          ; DX >>= 2 (dh = zero if no overflow, dl = high bits of cylinder)
    xchg ah, cl                         ; CL = Sector - 1, AH = 2
    inc cx                              ; CL = sector
    or cl, dl                           ; DL = Head; al = zero if no overflow
    jz .int13
.lba:
    mov ax, 0x4200                      ; Specify to use LBA
.int13:
    ; Okay, I promise THIS is the actual disk reading method
    inc ax                              ; AL = 1
    les bx, [bp-12]
    mov dl, [bp+2]
    push ss
    pop ds
    int 0x13
    mov sp, bp
    pop cx
    jnc .done
    loop .retry
.done:
    ; Restore regisetrs
    pop bx
    pop dx
    pop cx
    pop ax
    pop si
    pop di
    pop ds
    pop bp
    jc disk_error
    inc ax
    jnz .ret
    inc dx
.ret:
    ret

; args: SI
print:
    lodsb                               ; Load character
    or   al, al                         ; Is the character 0?
    jz   donePrint                      ; If so, return.
    mov  bx, 0007h                      ; If not, continue printing.
    mov  ah, 0Eh                        ; 0Eh = teletype output
    int  10h                            ; Call BIOS
    jmp  print                          ; We know we're not done printing yet, so repeat the function.
donePrint:
    ret

printerror:
    call print
.infinite:
    hlt
    jmp .infinite

; Variables and final signature

msg_diskerror:
    db "Disk error when trying to load.", 13, 10, 0
msg_error:
    db "No active partition was found.", 13, 10, 0
msg_notfound:
    db "The active partition is not bootable.", 13, 10, 0

times 0x1be-($-$$) db 0
partitions_layout:
    db 0x80
    db 0x02, 0x03, 0x00
    db 0x06
    db 0xFE, 0x3F, 0x0E
    dd 128
    dd 2498556

times 0x1FE-($-$$) db 0
dw 0xAA55