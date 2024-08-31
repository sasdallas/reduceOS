// fpu.h - header file for FPU manager

#ifndef FPU_H
#define FPU_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/hal.h>

// Functions
bool fpu_isSupportedCPUID();
int fpu_init();
void fpu_ftoa(char *buf, float f);

#endif
