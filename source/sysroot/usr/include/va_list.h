// va_list.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

// File operates in tandem with stdarg.h (va_list declaration here, macros in stdarg.h)

#ifndef VA_LIST_H
#define VA_LIST_H

// Declarations

typedef __builtin_va_list va_list;

#endif

