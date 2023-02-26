// panic.h - kernel panic handling

#ifndef PANIC_H
#define PANIC_H

// Includes
#include "include/libc/stdint.h" // Integer declarations like uint8_t.
#include "include/libc/stdbool.h" // Booleans
#include "include/terminal.h" // Terminal functions, like printf
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/regs.h" // registers_t typedef
#include "include/serial.h" // Serial logging


// Functions
void *panic(char *caller, char *code, char *reason);
void *panicReg(char *caller, char *code, char *reason, registers_t  *reg);
void *pageFault(registers_t  *reg);

#endif