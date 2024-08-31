#ifndef CONSOLE_H
#define CONSOLE_H

// Includes
#include <libk_reduced/stdint.h>
#include <libk_reduced/stdlib.h>
#include <libk_reduced/string.h>

// Variables
extern uint64_t boottime;
extern size_t (*printf_output)(size_t, uint8_t *);

// Functions
void console_init();
void console_setOutput(size_t (*output)(size_t,uint8_t*));
int console_printf(const char *fmt, ...);


#endif
