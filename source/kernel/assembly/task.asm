global start_process
global restore_kernel_selectors
global enter_tasklet
global restore_context
global save_context
global read_eip
global resume_usermode

; start_process - Takes in two arguments, process stack #1 and process entrypoint #2. Switches context to a process & starts it.
start_process:
	cli
	mov ax, (4 * 8) | 3
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Setup the stack frame that iret wants
	mov eax, [esp + 4]
	mov ebx, [esp + 8]
	push (4 * 8) | 3
	push eax
	pushf

	; As detailed in switch_to_usermode, we should setup interrupts
	; This is required for our task switching
	pop eax
	or eax, 0x200
	push eax

	push (3 * 8 | 3)
	push ebx
	iret

restore_kernel_selectors:
    cli
    mov eax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax 
    sti
    ret




read_eip:
	mov eax, esp
	ret

resume_usermode:
	; Resume into usermode from a thread (stored in thread EIP)
	; restore_context kindly gave us the stack we needed, so let's just pop whatever's in registers
    ; We're just basically doing what isrCommonStub does after an interrupt
	pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    popa
    add esp, 8
	iret