global start_process
global restore_kernel_selectors
global enter_tasklet

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

