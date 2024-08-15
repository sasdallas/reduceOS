global start_process
global restore_kernel_selectors
global enter_tasklet
global restore_context
global save_context

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

; save_context - our __builtin_setjmp replacement
save_context:
	mov ecx, [esp + 4]
	lea eax, [esp + 8]
	mov dword [ecx + 0], eax
	mov dword [ecx + 4], ebp
	xor eax, eax
	mov dword [ecx + 8], eax ; No one likes TLS_BASE (TODO: You can actually do this call with something like a rdmsr)
	mov eax, [esp + 0]
	mov dword [ecx + 12], eax

	; Basically, besides setting the above fields, we also need to do a setjmp-type thing.
	; Stuff the saved[5] structure with the following:
	; EBX, EDI, ESI, <reserved>, <reserved>
	mov dword [ecx + 16], ebx
	mov dword [ecx + 20], edi
	mov dword [ecx + 24], esi
	xor eax, eax
	mov dword [ecx + 28], eax 
	mov dword [ecx + 32], eax

	xor eax, eax
	ret

restore_context:
	mov ecx, [esp + 4]

	; Let's start by restoring SP and BP in the context.
	; We will jump to IP at the end, and I don't care about TLS_BASE (that's more aarch64 but it is in an MSR)
	mov dword esp, [ecx + 0]
	mov dword ebp, [ecx + 4]


	; Restore the context from saved[5]
	mov dword ebx, [ecx + 16]
	mov dword edi, [ecx + 20]
	mov dword esi, [ecx + 24]

	; Jump to IP
	jmp [ecx + 12]

