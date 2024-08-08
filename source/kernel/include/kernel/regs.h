// regs.h - simple header file that contains definitions of register structs

#ifndef REGS_H
#define REGS_H


// Includes
#include <stdint.h> // Integer declarations

// Typedefs
typedef struct REGISTERS {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed by pusha
    uint32_t int_no, err_code;                        // Interrupt # and error code
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by the processor automatically
} registers_t;

// Registers (16-bit real mode)
typedef struct {
    uint16_t di, si, bp, sp, bx, dx, cx, ax;
    uint16_t ds, es, fs, gs, ss;
    uint16_t eflags;
} REGISTERS_16;

// A special register struct for multitasking
typedef struct REGISTERS_MULTITASK {
    uint32_t ds, es;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed by pusha
    uint32_t int_no, err_code;                        // Interrupt # and error code
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by the processor automatically
} registers_multitask_t;

#endif
