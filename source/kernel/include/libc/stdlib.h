// stdlib.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STDLIB_H
#define STDLIB_H

// Functions:
int abs(int x); // Returns the absolute value of an integer (always positive)


#endif