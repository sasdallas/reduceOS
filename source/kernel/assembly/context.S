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
    movl 4(%esp), %ecx
    
    # Restoring from general context, SP/BP
    movl 0(%ecx), %esp
    movl 4(%ecx), %ebp

    # Restoring from saved[5]
    movl 16(%ecx), %ebx 
    movl 20(%ecx), %edi
    movl 24(%ecx), %esi
    #movl 28(%ecx), %ebp
    #movl 32(%ecx), %esp

    movl $1, %eax                           # This code is to make sure switchTask knows when we're back from a switch
    jmpl *12(%ecx)                          # Jump to IP
