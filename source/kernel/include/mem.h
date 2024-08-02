// mem.h - A header file for the entire memory management system
// It's not much, just a forwarder to the actual stuff ;)

#ifndef MEM_H
#define MEM_H

#include "include/liballoc_forwarder.h"


extern void *kmalloc(size_t);
extern void *krealloc(void *, size_t);
extern void *kcalloc(size_t, size_t);
extern void kfree(void *);




#endif