// liballoc_forwarder.h - Reskinned heap header file for old code

#ifndef LIBALLOC_FORWADER_H
#define LIBALLOC_FORWARDER_H

// Includes
#include <stdint.h> // Integer declarations
#include <string.h> // String functions
#include <assert.h>
#include <kernel/vmm.h>

// Macros
#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)



// (variables)
extern uint32_t end; // end is defined in the linker.ld script we added (thanks James Molloy!)
extern bool pagingEnabled;

// Functions

#endif
