#ifndef CONSOLE_H
#define CONSOLE_H

// Includes
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Variables
extern uint64_t boottime;
extern size_t (*printf_output)(size_t, uint8_t *);

// Functions
void console_init();
void console_setOutput(size_t (*output)(size_t,uint8_t*));
int console_printf(const char *fmt, ...);


#endif