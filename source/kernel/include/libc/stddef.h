// stddef.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

// Macros
#define null 0

// Typedef declarations
typedef unsigned int size_t;
typedef signed ptrdiff_t;
typedef wchar_t;
