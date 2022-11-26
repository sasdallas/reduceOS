// panic.h - kernel panic handling

#ifndef PANIC_H
#define PANIC_H

// Includes
#include "include/libc/stdint.h"
#include "include/libc/stdbool.h"
#include "include/terminal.h"
#include "include/kernel.h"

// Functions
void *panic(char *caller, char *code, char *reason);


#endif