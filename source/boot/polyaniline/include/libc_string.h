// libc_string.h - Ported straight from libc_reduced

#ifndef LIBC_STRING_H
#define LIBC_STRING_H
#include <libc_types.h> // Integer declarations, booleans, size_t, all of it
#include <stddef.h>


// TECHNICALLY this isn't supposed to be here but I don't care that much
#define EOF -1

// no function prototypes... they cause issues in the kernel :/
// except for one
void *memchr(const void *src, int c, size_t n);
#endif