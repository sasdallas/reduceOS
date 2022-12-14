// panic.h - kernel panic handling

#ifndef PANIC_H
#define PANIC_H

// Includes
#include "include/libc/stdint.h" // Integer declarations like uint8_t.
#include "include/libc/stdbool.h" // Booleans
#include "include/terminal.h" // Terminal functions, like printf
#include "include/hal.h" // Hardware Abstraction Layer

// Typedefs
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushed by pusha
    uint32_t int_no, err_code;                        // Interrupt # and error code
    uint32_t eip, cs, eflags, useresp, ss;            // pushed by the processor automatically
} REGISTERS;


// Functions
void *panic(char *caller, char *code, char *reason);
void *panicReg(char *caller, char *code, char *reason, REGISTERS *reg);


#endif