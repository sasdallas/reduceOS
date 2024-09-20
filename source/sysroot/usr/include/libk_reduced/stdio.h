// stdio.h - Handles outputting to console or other devices

#ifndef __INTELLISENSE__
#if !defined __KERNEL__ && !defined __KERNMOD__
#error "This file cannot be used in userspace."
#endif
#endif


#ifndef STDIO_H
#define STDIO_H

// Includes
#include <kernel/terminal.h> // TODO: Implement ability to use stdio and other stuff

// Typedefs (NOT STANDARD)
typedef int (*xvas_callback)(void *, char);

// lseek() parameters
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

// Functions
int printf(const char * __restrict, ...);
int putchar(int);
int puts(const char *);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int snprintf(char * str, size_t size, const char * format, ...);
int sprintf(char * str, const char * format, ...);
size_t xvasprintf(xvas_callback callback, void * userData, const char * fmt, va_list args);

#endif
