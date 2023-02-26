; -------------------------------------------
; memory.inc - Getting memory information
; -------------------------------------------

BITS 16                                 ; We are still 16-bit

%include "include/panic16.inc"              ; Being able to handle if the BIOS fails to get the memory map

; Defining the memory map entry sturcture
struc MemoryMapEntry
    .baseAddress resq 1
    .length resq 1
    .type resd 1
    .acpiNull resd 1
endstruc


; =================================================
; biosGetMemoryMap - Get memory map from BIOS
; Parameters: ES:DI (destination buffer)
; Returns: BP (entry count)
; =================================================

biosGetMemoryMap:
    xor ebx, ebx                        ; Clear EBX and BP (our outputs)
    xor bp, bp
    mov edx, 0x0534D4150                ; Place "SMAP" into EDX
    mov eax, 0xE820
    mov [es:di + 20], dword 1           ; We need to force a valid ACPI 3.x entry
    mov ecx, 24
    int 0x15                            ; Call BIOS
    jc short .failed                    ; Carry set on first call means "unsupported function". We cannot load reduceOS.
    mov edx, 0x0534D4150                ; Some BIOSes trash this register
    cmp eax, edx                        ; On success EAX is reset to "SMAP"
    jne short .failed                   ; We failed, big sad.
    test ebx, ebx                       ; If EBX is 0, the map is only 1 entry long, meaning its worthless
    je short .failed
    jmp short .jmpin
.e820lp:
    mov eax, 0xe820                     ; EAX and ECX get trashed on every INT 0x15 call.
    mov [es:di + 20], dword 1           ; Force a valid ACPI 3.x entry again.
    mov ecx, 24                         ; Ask for 24 bytes
    int 0x15
    jc short .e820f                     ; Carry flag set means end of list already reached.
    mov edx, 0x0534D4150                ; Some BIOSes trash this register.
.jmpin:
    jcxz .skipent                       ; Skip any 0 length entries
    cmp cl, 20                          ; Did we get a 24 byte response?
    jbe short .notext
    test byte [es:di + 20], 1           ; If so, is the ignore this data bit clear?
    je short .skipent
.notext:
    mov ecx, [es:di + 8]                ; Get lower DWORD of memory region length
    test ecx, ecx                       ; Is the QWORD 0?
    jne short .goodent
    
    mov ecx, [es:di + 12]               ; Get upper DWORD of memory region length
    jecxz .skipent                      ; Is length QWORD 0? If so, skip entry.
.goodent:
    inc bp                              ; Yay! We got a good entry. Increase BP (count) and move on (add 24 to DI)
    add di, 24
.skipent:
    test ebx, ebx                       ; If EBX resets to 0, list is complete
    jne short .e820lp
.e820f:
    ret                                 ; Return. We are done (BP = entry count)
.failed:
    mov si, FUNCTION_UNSUPPORTED        ; We failed to get the memory map. Terminate the system.
    call panic

    ; Just in case something weird happens, make sure we also terminate the system
    stc
    ret


; ===============================================================================
; biosGetMemorySize64MB_32 - Get memory size for >64M configurations (32-bit)
; Returns - AX (KB between 1MB and 16MB)
;         - BX (number of 64K blocks >16MB)
;         - BX = 0 and AX = -1 on error.
; ===============================================================================

biosGetMemorySize64MB_32:
    push ecx                            ; Save the ECX and EDX registers
    push edx
    xor ecx, ecx                        ; Clear ECX and EDX
    xor edx, edx

    mov eax, 0xE881                     ; Move EAX to the proper value for INT 0x15
    int 0x15                            ; Call BIOS

    jc .error                           ; Fail if error occurred.
    jcxz .useAX                         ; BIOS may have stored in AX:BX or CX:DX. Test if CX is 0
    mov eax, ecx                        ; It's not, so it should contain memory size - store it.
    mov ebx, edx 

.useAX:
    pop edx                             ; Memory size is in AX and BX already - return it.
    pop ecx
    ret

.error:
    mov ax, -1                          ; This isn't a critical failure, so we don't need to panic.
    mov bx, 0
    pop edx
    pop ecx
    ret

; ===============================================================================
; biosGetMemorySize64MB - Get memory size for >64M configurations (16-bit)
; Returns - AX (KB between 1MB and 16MB)
;         - BX (number of 64K blocks >16MB)
;         - BX = 0 and AX = -1 on error.
; ===============================================================================

biosGetMemorySize64MB:
    push ecx                            ; Save ECX and EDX
    push edx
    xor ecx, ecx                        ; Clear ECX and EDX
    xor edx, edx

    mov ax, 0xE801                      ; Set EAX to the proper value for INT 0x15
    int 0x15                            ; Call BIOS
    jc .error
    cmp ah, 0x86                        ; Unsupported function.
    je .error
    cmp ah, 0x80                        ; Invalid command.
    je .error
    jcxz .useAX                         ; BIOS could of stored in AX:BX or CX:DX. Test if CX is 0.
    mov ax, cx                          ; It's not. Store in AX and BX.
    mov bx, dx

.useAX:
    pop edx                             ; Already stored in AX:BX. No need to do anything else.
    pop ecx
    ret

.error:
    mov ax, -1                          ; Not a critical failure, don't panic.
    mov bx, 0
    pop edx
    pop ecx
    ret


; ==========================================================================
; biosGetMemorySize - Get amount of contiguous kilobytes from addr 0
; Returns - AX (KB size from address 0)
; ==========================================================================

biosGetMemorySize:
    int 0x12
    ret


; ==========================================================================
; biosGetExtendedMemorySize - Get contigouous extended memory size.
; Returns - AX (KB size above 1MB)
;         - AX = -1 (on error)
; ==========================================================================

biosGetExtendedMemorySize:
    mov ax, 0x88
    int 0x15
    jc .error                           ; Failed - exit.
    test ax, ax                         ; Is size 0?
    je .error
    cmp ah, 0x86                        ; 0x86 means unsupported function.
    je .error
    ret
.error:
    mov ax, -1                          ; Not critical - don't panic.
    ret




FUNCTION_UNSUPPORTED db "Function unsupported (loading memory map).", 0x0D, 0x0A, 0x00

