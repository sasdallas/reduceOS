// page.h (i386 architecture specific)

#if !defined(ARCH_I386) && !defined(__INTELLISENSE__) 
#error "This file is for the i386 architecture" 
#endif

#ifndef ARCH_I386_PAGE_H
#define ARCH_I386_PAGE_H

// Includes
#include <libk_reduced/stdint.h>

// Definitions
#define PAGE_SHIFT 12       // This is the amount to shift page addresses to get the index of the PTE


#endif 