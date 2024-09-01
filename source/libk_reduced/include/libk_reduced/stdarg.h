// stdarg.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

// File operates in tandem with va_list.h (va functions/macros here, whilst the actual va_list declaration is in va_list.h)


#ifndef STDARG_H
#define STDARG_H

// Includes of other header files
#include <libk_reduced/va_list.h>

// Definitions (non-macro)
#define STACKITEM int // Width of stack (width of an int)

// Macros

// VA_SIZE(TYPE) - Round up width of objects pushed on stack.
#define	VA_SIZE(TYPE)					\
	((sizeof(TYPE) + sizeof(STACKITEM) - 1)	\
		& ~(sizeof(STACKITEM) - 1))



#define va_start(x, y) __builtin_va_start(x, y)
#define va_arg(x, y) __builtin_va_arg(x, y)
#define va_end(x) __builtin_va_end(x)
#endif
