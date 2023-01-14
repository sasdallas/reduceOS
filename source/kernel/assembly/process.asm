; process.asm - Handles physical page management and a few other things.
; This implementation is sourced from JamesM's kernel development tutorials.

[global copyPagePhysical]

copyPagePhysical:
    push ebx                    ; Make sure we push ebx.
    pushf                       ; Push EFLAGS so we can pop it and reenable interrupts later.

    cli                         ; Disable interrupts
    
    mov ebx, [esp+12]           ; Source address.
    mov ecx, [esp+16]           ; Destination address.

    mov edx, cr0                ; Get control register.
    and edx, 0x7fffffff         
    mov cr0, edx                ; Disable paging.

    mov edx, 1024               ; 1024 * 4 bytes = 4096 bytes to copy.

.loop:
    mov eax, [ebx]              ; Get word at the source address.
    mov [ecx], eax              ; Store at destination address.
    add ebx, 4                  ; Add sizeof(word) to source and destination address
    add ecx, 4
    dec edx                     ; Decrement (we completed 1 word)
    jnz .loop                   ; Keep going.

    mov edx, cr0                ; Get the control register.
    or edx, 0x80000000   
    mov cr0, edx                ; Enable paging again.

    popf                        ; Pop EFLAGS.
    pop ebx
    ret
    
