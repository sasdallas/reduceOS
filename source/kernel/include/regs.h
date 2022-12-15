// regs.h - simple header file that contains definitions of registery sttructs

#ifndef REGS_H
#define REGS_H


// Includes
#include "include/libc/stdint.h" // Integer declarations

// Typedefs
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed by pusha
    uint32_t int_no, err_code;                        // Interrupt # and error code
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by the processor automatically
} REGISTERS;


#endif