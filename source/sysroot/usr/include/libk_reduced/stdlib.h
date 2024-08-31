// stdlib.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STDLIB_H
#define STDLIB_H

// Functions:
int abs(int x); // Returns the absolute value of an integer (always positive)
double min(double n1, double n2); // Returns the smallest of the parameters
double max(double n1, double n2); // Returns the largest of the parameters
double sqrt(double n); // Returns the sqrt of a number (return value ^ 2 = number)


#endif
