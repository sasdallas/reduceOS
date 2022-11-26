// string.h - replacement for standard C header file. Contains certain useful functions like strlen.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STRING_H
#define STRING_H

#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/stdbool.h" // Boolean declarations
#include "include/libc/stddef.h" // size_t declaration



// Function definitions
int memcmp(const void*, const void*, size_t); // memcmp() - Comparing addresses/values in memory
void* memcpy(void* __restrict, const void* __restrict, size_t); // memcpy() - copy a block of data from one address to another.
void* memmove(void*, const void*, size_t); // memmove() - memcpy but moving it instead of copying.
size_t strlen(const char* len); // strlen() - Returns the length of a string.
char* itoa(int num, char* buffer, int base); // itoa() - converts an integer to a string.
char *strcpy(char *dest, const char *src); // strcpy() - copies one string to another

#endif
