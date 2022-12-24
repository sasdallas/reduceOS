// panic.h - kernel panic handling

#ifndef PANIC_H
#define PANIC_H

// Includes
#include "include/libc/stdint.h" // Integer declarations like uint8_t.
#include "include/libc/stdbool.h" // Booleans
#include "include/terminal.h" // Terminal functions, like printf
#include "include/hal.h" // Hardware Abstraction Layer
#include "include/regs.h" // REGISTERS typedef



// Functions
void *panic(char *caller, char *code, char *reason);
void *panicReg(char *caller, char *code, char *reason, REGISTERS *reg);
void *specialPanic(char *caller, char *code, char *reason, int integer, uint32_t address, char *data1desc, char *data2desc);
void *pageFault(REGISTERS *reg);

#endif