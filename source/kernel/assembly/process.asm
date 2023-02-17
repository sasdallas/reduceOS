; process.asm - Handles task context switching and a few other things.
; This implementation is sourced from eduOS by RWTH-OS

[global task_switchContext]
[extern task_getCurrentStack]
[extern setKernelStack]
[extern task_finishTaskSwitch]

ALIGN 4

task_switchContext:
    ; Create on the stack a "psuedo-interrupt", afterwards switch to the task.
    ; Note: We are already in kernel space so no pushing of ss required.
    mov eax, [esp+4]            ; On the stack is already the address to store the old esp.
    pushf                       ; Push control register.
    push DWORD 0x8              ; Push CS, EIP, INT_NO, ERR_CODE, and all general purpose registers.
    push DWORD rollback
    push DWORD 0x0
    push DWORD 0x00EDBABE
    pusha
    push 0x10                   ; Push the two kernel data segments
    push 0x10

    jmp commonSwitch


ALIGN 4
rollback:
    ret



commonSwitch:
    mov [eax], esp            ; Store old ESP
    call task_getCurrentStack ; Get new ESP
    xchg eax, esp

    ; Set the task switched flag.
    mov eax, cr0
    or eax, 8
    mov cr0, eax

    ; Set esp0 in the TSS
    call setKernelStack

    ; Call cleanup code
    call task_finishTaskSwitch

task_noContextSwitch:
    pop ds
    pop es
    popa
    add esp, 8
    iret