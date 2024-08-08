// string.h - replacement for standard C header file. Contains certain useful functions like strlen.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STRING_H
#define STRING_H

#include <stdint.h> // Integer declarations
#include <stdbool.h> // Boolean declarations
#include <stddef.h> // size_t declaration
#include <limits.h> // LONG_MIN and LONG_MAX
#include <kernel/mem.h>

// TECHNICALLY this isn't supposed to be here but I don't care that much
#define EOF -1

// Function definitions
int memcmp(const void*, const void*, size_t); // memcmp() - Comparing addresses/values in memory
void* memcpy(void* __restrict, const void* __restrict, size_t); // memcpy() - copy a block of data from one address to another.
void* memmove(void*, const void*, size_t); // memmove() - memcpy but moving it instead of copying.
void *memset(void* buf, char c, size_t n); // memset() - set a buffer in memory to given value
void itoa(void *num, char* buffer, int base); // itoa() - converts an integer to a string. stores in buffer so no return value.
void itoa_long(uint64_t num, char *str, int base); // itoa_long() - Dirty hack to fix a bug
char *strcpy(char *dest, const char *src); // strcpy() - copies one string to another
char toupper(char c); // toupper() - turns a character uppercase
char tolower(char c); // tolower() - turns a character lowercase
int isalpha(char ch); // isalpha() -  returns if a char is in the alphabet
int strcmp(const char *str1, char *str2); // strcmp() - compares a string.
int strncmp(const char *str1, char *str2, int length); // strncmp() - compares a string for x amount of chars.
int strlen(char *str); // strlen() - checks length of a string
char *strtok(char *str, const char *delim); // strtok() - splits a string into tokens, seperated by delim.
long strtol(const char *nptr, char **endptr, int base); // strtol() - <todo add description>
int atoi(char *str); // atoi() - string to integer
char *strchr(char *str, int ch); // Locate the first occurance of a character in a string (credit to BrokenThorn Entertainment)
char *strchrnul(const char *str, int ch); // Locates a character in a string
size_t strcspn(const char *str1, const char *reject); // Scans str1 for the first occurence of any of the characters that are NOT part of reject.
size_t strspn(const char *str1, const char *accept); // Scans str1 for the first occurence of any of the characters that are part of accept.
char *strpbrk(const char *s, const char *b); // Returns a pointer to the first occurence of b within s.
char *strtok_r(char *str, const char *delim, char **saveptr); // Thread-safe version of strtok

#endif