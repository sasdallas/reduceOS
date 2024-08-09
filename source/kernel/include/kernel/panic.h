// panic.h - kernel panic handling

#ifndef PANIC_H
#define PANIC_H

// Includes
#include <stdint.h> // Integer declarations like uint8_t.
#include <stdbool.h> // Booleans
#include <kernel/terminal.h> // Terminal functions, like printf
#include <kernel/hal.h> // Hardware Abstraction Layer
#include <kernel/regs.h> // registers_t typedef
#include <kernel/serial.h> // Serial logging
#include <kernel/CONFIG.h> // Configuration

// Typedefs
typedef struct {
    struct stack_frame* ebp;
    uint32_t eip;
} stack_frame;

// Functions
void *panic(char *caller, char *code, char *reason);
void *panicReg(char *caller, char *code, char *reason, registers_t  *reg);
void *pageFault(registers_t  *reg);

#endif
