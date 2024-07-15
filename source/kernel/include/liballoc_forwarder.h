// liballoc_forwarder.h - Reskinned heap header file for old code

#ifndef LIBALLOC_FORWADER_H
#define LIBALLOC_FORWARDER_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/string.h" // String functions
#include "include/libc/assert.h"

// Macros
#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)



// (variables)
extern uint32_t end; // end is defined in the linker.ld script we added (thanks James Molloy!)

// Functions

#endif