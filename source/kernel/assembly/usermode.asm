global switchToUserMode
extern usermodeMain

switchToUserMode:
	cli 					; We don't want an interrupt to mess this up

	; Setup segments
    mov ax, (4 * 8) | 3 	; Ring data with bottom 2 bits set for ring 3 (0x23)
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax 				; SS is handled by iret
 
	; Setup the stack frame that IRET will expect
	mov eax, esp
	push (4 * 8) | 3 		; Data selector (0x23)
	push eax 				; Current ESP
	pushf 					; EFLAGS

	; We want to re-enable interrupts, so we have to set IF in EFLAGS
	pop eax					; Restore EAX (only way to read EFLAGS is pushf then pop)
	or eax, 0x200			; Bitmask for IF flag
	push eax				; Push EFLAGS back onto the stack
	push (3 * 8) | 3 		; Code selector (0x1B - ring 3 code with bottom 2 bits set for ring 3)
	push usermodeMain 		; Instruction address to return to
	iret


