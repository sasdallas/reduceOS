// mem.h - A header file for the entire memory management system
// It's not much, just a forwarder to the actual stuff ;)

#ifndef MEM_H
#define MEM_H

// Includes
#include <stdint.h> // Integer declarations
#include <string.h> // String functions
#include <assert.h>
#include <kernel/vmm.h>

// Macros
#define ALIGN_PAGE(addr) (((uint32_t)addr & 0xFFFFF000) + 4096)

// External variables
extern uint32_t end;
extern bool pagingEnabled;

// Functions
void enable_liballoc();
void *kmalloc(size_t size);
void *krealloc(void *a, size_t b);
void *kcalloc(size_t a, size_t b);
void kfree(void *a);

#endif
