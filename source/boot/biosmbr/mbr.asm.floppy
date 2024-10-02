; ============================================================
; mbr.asm - The Master Boot Record loader for Polyaniline
; ============================================================
; (c) sasdallas, 2024
; The FAT12 and BPB code is sourced from Brokenthorn's MBR.

org 0x0                                 ; Loaded in by BIOS at 0x0
bits 16                                 ; We start at 16-bit

start: jmp main                         ; Jump to main

; OEM Parameter Block for FAT/MBR
TIMES 0Bh-$+start DB 0
 
bpbBytesPerSector:  	DW 512
bpbSectorsPerCluster: 	DB 1
bpbReservedSectors: 	DW 1
bpbNumberOfFATs: 	DB 2
bpbRootEntries: 	DW 224
bpbTotalSectors: 	DW 2880
bpbMedia: 	        DB 0xF0
bpbSectorsPerFAT: 	DW 9
bpbSectorsPerTrack: 	DW 18
bpbHeadsPerCylinder: 	DW 2
bpbHiddenSectors:       DD 0
bpbTotalSectorsBig:     DD 0
bsDriveNumber: 	        DB 0
bsUnused: 	        DB 0
bsExtBootSignature: 	DB 0x29
bsSerialNumber:	        DD 0xa0a1a2a3
bsVolumeLabel: 	        DB "polyaniline"
bsFileSystem: 	        DB "FAT12   "

print:
  mov ah, 0eh
.loop:
  lodsb                                 ; Load character from SI buffer
  or al, al                             ; Is it a zero?
  jz .print_done                        ; Done printing :D 
  int 10h                               ; Call BIOS
  jmp .loop                             ; Loop!
.print_done:
  ret                                   ; Return  

read_sectors:
.main:
  mov di, 0x0005                        ; Allow 5 retries
.sectorloop:
  push ax
  push bx
  push cx
  call LBACHS                           ; Convert starting sector to CHS
  mov ah, 0x02                          ; BIOS read sector
  mov al, 0x01                          ; Read one sector
  mov ch, BYTE [absoluteTrack]          ; Track
  mov cl, BYTE [absoluteSector]         ; Sector
  mov dh, BYTE [absoluteHead]           ; Head
  mov dl, BYTE [bsDriveNumber]          ; Drive
  int 0x13                              ; Call BIOS
  jnc .success
  xor ax, ax                            ; BIOS reset disk
  int 0x13                              ; Invoke BIOS
  dec di                                ; Decrement error counter
  pop cx
  pop bx
  pop ax
  jnz .sectorloop                       ; Attempt to read again
  int 0x18
.success:
  mov si, msg_progress
  call print
  pop cx
  pop bx
  pop ax
  add bx, WORD [bpbBytesPerSector]      ; Queue next buffer & sector
  inc ax
  loop .main
  ret

cluster_lba:
  sub ax, 0x0002                        ; Zero base cluster number
  xor cx, cx
  mov cl, BYTE [bpbSectorsPerCluster]   ; Byte -> Word
  mul cx
  add ax, WORD [datasector]             ; Base data sector 
  ret


; LBACHS - Converts LBA to CHS
; Absolute Sector = (logical sector / sectors per track) + 1
; Absolute Head = (logical sector / sectors per track) % number of heads
; Absolute Track = logical sector / (sectors per track * number of heads)
LBACHS:
  xor dx, dx                            ; Prepare DX:AX for operation
  div WORD [bpbSectorsPerTrack]         ; Calculate
  inc dl                                ; Adjust for sector 0
  mov BYTE [absoluteSector], dl
  xor dx, dx                            ; Prepare DX:AX for operation
  div WORD [bpbHeadsPerCylinder]        ; Calculate
  mov BYTE [absoluteHead], dl
  mov BYTE [absoluteTrack], al
  ret


  
main:
  ; Adjust segment registers for 0000:7c00 code boundary
  cli
  mov ax, 0x07c0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; Create stack and restore interrupts
  mov ax, 0x0000
  mov ss, ax
  mov sp, 0xFFFF
  sti

  ; Print loading message
  mov si, loading_msg
  call print

  ; Load root directory table
  LOAD_ROOT:
  ; Compute size of root directory -> CX
  xor cx, cx
  xor dx, dx
  mov ax, 0x0020                        ; 32 byte directory entry
  mul WORD [bpbRootEntries]             ; Total size of directory
  div WORD [bpbBytesPerSector]          ; Sectors used by directory
  xchg ax, cx

  ; Compute location of root directory -> AX
  mov al, BYTE [bpbNumberOfFATs]        ; Number of FATs
  mul WORD [bpbSectorsPerFAT]           ; Sectors used by FATs
  add ax, WORD [bpbReservedSectors]     ; Adjust for boot sector
  mov WORD [datasector], ax             ; Base of root directory
  add WORD [datasector], cx

  ; Read root directory into memory (7c00:0200)
  mov bx, 0x0200
  call read_sectors

  ; Locate BOOTLDR.SYS
  mov cx, WORD [bpbRootEntries]         ; Load loop counter
  mov di, 0x0200                        ; Locate first root entry
.locate_loop:
  push cx
  mov cx, 0x000B                        ; 11 character name
  mov si, ImageName                     ; Image name to find
  push di
  rep cmpsb                             ; Check for match
  pop di
  je LOAD_FAT
  pop cx
  add di, 0x0020                        ; Queue next entry
  loop .locate_loop
  jmp FAILED

  ; Now load the FAT
  LOAD_FAT:

  ; Save the starting cluster of the boot image
  mov si, msg_crlf
  call print
  mov dx, WORD [di + 0x001A]
  mov WORD [cluster], dx               ; File's first cluster

  ; Compute size of FAT -> CX
  xor ax, ax
  mov al, BYTE [bpbNumberOfFATs]      ; Number of FATs
  mul WORD [bpbSectorsPerFAT]         ; Sectors used by FATs
  mov cx, ax

  ; Compute location of FAT -> AX
  mov ax, WORD [bpbReservedSectors]  ; Adjust for reserved sectors

  ; Read FAT into memory above the bootcode
  mov bx, 0x0200                     ; Destination -> 0x0200 
  call read_sectors

  ; Read image file into memory
  mov si, msg_crlf
  call print
  mov ax, 0x0050
  mov es, ax                        ; Destination for image
  mov bx, 0x0000                    ; Destination for image
  push bx

  ; Load stage 2
  LOAD_IMAGE:
  mov ax, WORD [cluster]            ; Cluster to read
  pop bx                            ; Buffer to read into
  call cluster_lba                  ; Convert cluster to LBA
  xor cx, cx
  mov cl, BYTE [bpbSectorsPerCluster] ; Sectors to read
  call read_sectors
  push bx

  ; Compute next cluster
  mov ax, WORD [cluster]            ; Identify current cluster
  mov cx, ax                        ; Copy current cluster
  mov dx, ax
  shr dx, 0x0001                    ; Divide it by 2
  add cx, dx                        ; Sum for (3/2)
  mov bx, 0x0200                    ; Location of FAT into memory
  add bx, cx                        ; Index into FAT
  mov dx, WORD [bx]                 ; Read two bytes from FAT
  test ax, 0x0001
  jnz .odd_cluster

.even_cluster:
  and dx, 0000111111111111b         ; Take the low 12 bits
  jmp .done

.odd_cluster:
  shr dx, 0x0004                    ; Take the high 12 bits

.done:
  mov WORD [cluster], dx            ; Store new cluster
  cmp dx, 0x0FF0                    ; Test for end of file
  jb LOAD_IMAGE
  
  DONE_LOADING:
  mov si, msg_crlf
  call print
  push word 0x0050
  push word 0x0000
  retf

  FAILED:
  ; Failed to locate image :(
  mov si, load_fail
  call print

  cli                                   ; Clear interrupts
  hlt                                   ; Halt system


; Variables

absoluteSector db 0x00
absoluteHead db 0x00
absoluteTrack db 0x00

datasector dw 0x0000
cluster dw 0x0000
ImageName db "BOOT    SYS"
msg_crlf db 0x0D, 0x0A, 0x00
msg_progress db ".", 0x00


loading_msg db 0x0D, 0x0A, "Loading BOOT.SYS...", 0x0D, 0x0A, 0
load_fail db 0x0D, 0x0A, "ERROR: Load failed, halting.", 0x0D, 0x0A, 0


times 510 - ($-$$) db 0                 ; Padding for size
dw 0xAA55                               ; Boot signature for BIOS