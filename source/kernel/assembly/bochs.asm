; bochs.asm - Just contains a macro for breakpointing on Bochs


section .text:
    global bochs_break

bochs_break:
    xchg bx, bx     ; this crashes for some reason on the emulator