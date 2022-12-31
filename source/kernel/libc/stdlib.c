// stdlib.c - replacement for standard C stdlib file. Contains certain useful functions like abs().
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#include "include/libc/stdlib.h"

int abs(int x) {
    if (x < 0) x = -x;
    return x;
}