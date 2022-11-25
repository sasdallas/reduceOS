// stdarg.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

// File operates in tandem with va_list.h (va functions/macros here, whilst the actual va_list declaration is in va_list.h)

#ifndef STDARG_H
#define STDARG_H

// Includes of other header files
#include "include/libc/va_list.h" // I mean, it would be faster to do #include "va_list.h", but it wouldn't compile (-Isource/kernel flag). Makes VS code pretty angry, that's for sure.

// Definitions (non-macro)
#define STACKITEM int // Width of stack (width of an int)

// Macros

// VA_SIZE(TYPE) - Round up width of objects pushed on stack.
#define VA_SIZE(TYPE)   \
    ((sizeof(TYPE) + sizeof(STACKITEM)) \
        & ~(sizeof(STACKITEM) - 1)) // The expression before '&' ensures we get 0 for objects of size 0.

// va_start(AP, LASTARG) - Initializes 'AP' to retrieve additional arguments after LASTARG (the leftmost argument, before the '...')
#define va_start(AP, LASTARG)   \
    (AP=((va_list)&(LASTARG) + VA_SIZE(LASTARG)))

// va_end(AP) - Nothing lol
#define va_end(AP)

// va_arg(AP, TYPE) - Retrieve the next argument of type 'TYPE' in va_list 'AP'
#define va_arg(AP, TYPE)    \
    (AP += VA_SIZE(TYPE), *((TYPE *)(AP - VA_SIZE(TYPE))))


#endif