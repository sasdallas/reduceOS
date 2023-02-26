; ==================================================
; paging.inc - Handles paging for bootloader
; ==================================================


BITS 32                                             ; Start in 32-bits

%define PAGE_DIR 0x9C000 ; Page directory table
%define PAGE_TABLE_0 0x9D000 ; 0th page table (address must be 4KB aligned)
%define PAGE_TABLE_768 0x9E000 ; 768th page table (address must be 4KB aligned)
%define PAGE_TABLE_ENTRIES 1024 ; Each page table has 1024 entries
%define PRIV 3 ; Attributes


; ---------------------------------------
; enablePaging - enables paging
; ---------------------------------------

enablePaging:
    pusha                                           ; Save stack frame

    mov eax, PAGE_TABLE_0                           ; First page table
    mov ebx, 0x0 | PRIV                             ; EBX stores the starting physical address of page
    mov ecx, PAGE_TABLE_ENTRIES                     ; For every page in the table.

.loop:
    mov dword [eax], ebx                            ; Write the entry
    add eax, 4                                      ; Go to next page entry in table - remember, each entry is 4 bytes
    add ebx, 4096                                   ; Go to next page address - remember, each address must be 4k alignned
    loop .loop

    mov eax, PAGE_TABLE_0 | PRIV                    ; 1st table is directory entry 0
    mov dword [PAGE_DIR], eax

    mov eax, PAGE_TABLE_768 | PRIV                  ; 768th entry in directory table
    mov dword [PAGE_DIR+(768*4)], eax

    mov eax, PAGE_DIR
    mov cr3, eax

    mov eax, cr0                                    ; Enable paging
    or eax, 0x80000000
    mov cr0, eax

    ; Map the 768th table to physical address 1MB.
    ; The 768th table starts the 3 GB virtual address

    mov eax, PAGE_TABLE_768                         ; First page table.
    mov ebx, 0x10000 | PRIV                         ; Starting physical address of the page
    mov ecx, PAGE_TABLE_ENTRIES                     ; For every page in the table

.loop2:
    mov dword [eax], ebx                            ; Write the entry.
    add eax, 4                                      ; Go to next page entry in table - remember, each entry is 4 bytes
    add ebx, 4096                                   ; Go to next page address - remember, each address must be 4k alignned
    loop .loop2                                     ; Loop!

    popa                                            ; Restore registers and return.
    ret