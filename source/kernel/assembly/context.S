# context.S - handles saving/loading context
# This code takes in a thread's context as an input, loads it with registers, and voila

.global save_context
.type save_context, @function
save_context:
    movl 4(%esp), %ecx
    leal 8(%esp), %eax
    movl %eax, 0(%ecx)
    movl %ebp, 4(%ecx)
    xorl %eax, %eax
    movl %eax, 8(%ecx)
    movl (%esp), %eax
    movl %eax, 12(%ecx)

    movl %ebx, 16(%ecx)
    movl %edi, 20(%ecx)
    movl %esi, 24(%ecx)
    movl %ebp, 28(%ecx)
    movl %esp, 32(%ecx)

    xor %eax, %eax
    retl



.global load_context
.type load_context, @function
load_context:
    # Usermode shenanigans
    mov %gs:0x10, %eax
    cmp %gs:0x0, %eax
    je 1f
    # lock andl $0xFFFFfff7, 0x14(%eax)
1:
    movl 4(%esp), %edi
    
    # Restoring from general context, SP/BP
    movl 0(%edi), %esp
    movl 4(%edi), %ebp

    # Restoring from saved[5], note we don't actually do basically any of it.
    movl 16(%edi), %ebx 

    movl $1, %eax                           # This code is to make sure switchTask knows when we're back from a switch
    jmpl *12(%edi)                          # Jump to IP