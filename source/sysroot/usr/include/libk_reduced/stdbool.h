// stdbool.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STDBOOL_H
#define STDBOOL_H

#define true 1
#define false 0
#define __bool_true_false_are_defined 1

typedef enum {
    FALSE,
    TRUE
} bool;

#endif
