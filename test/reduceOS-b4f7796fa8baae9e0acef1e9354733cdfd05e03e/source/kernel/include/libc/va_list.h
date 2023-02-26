// va_list.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

// File operates in tandem with stdarg.h (va_list declaration here, macros in stdarg.h)

#ifndef VA_LIST_H
#define VA_LIST_H

// Declarations

typedef unsigned char *va_list; // Parameter list. Used mainly with functions that have "..." in their parameter list (unlimited parameters)

#endif
