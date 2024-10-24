// libc_string.h - Ported straight from libc_reduced

#ifndef LIBC_STRING_H
#define LIBC_STRING_H
#include <libc_types.h> // Integer declarations, booleans, size_t, all of it
#include <stddef.h>


// TECHNICALLY this isn't supposed to be here but I don't care that much
#define EOF -1

// EFI function types cause issues in the kernel
#ifdef BIOS_PLATFORM
void *memchr(const void *src, int c, size_t n);

// and maybe a few others
int memcmp(const void *s1, const void *s2, size_t n);
void* memcpy(void* __restrict destination, const void* __restrict source, size_t n);
void* memmove(void* destination, const void* source, size_t n);
void* memset(void *buf, char c, size_t n);
#endif

#endif