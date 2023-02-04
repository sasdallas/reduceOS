.section .init
.global _init
.type _init, @function
_init:
    push %ebp
    movl %esp, %ebp
    # GCC will put the contents of crtbegin.o's .init section here.

.section .fini
.global _fini
.type _fini, @function
_fini:
    push %ebp
    mov %esp, %ebp
    # GCC will put the contents of crtbegin.o's .fini section here.