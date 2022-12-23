// stddef.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.


// Definitions
#define null 0

// Typedef declarations
typedef unsigned size_t;
typedef signed ptrdiff_t;
typedef wchar_t;
