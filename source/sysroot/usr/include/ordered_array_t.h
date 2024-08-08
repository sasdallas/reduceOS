// ordered_array_t.h - Contains the definition for ordered_array_t
#ifndef ORDERED_ARRAY_T_H
#define ORDERED_ARRAY_T_H

#include <stdint.h>

typedef void* type_t; // This array is insertion sorted, meaning it always remains in a sorted state between calls.

typedef int8_t (*lessthan_predicate)(type_t, type_t); // A predicate should return non-zero if the first arg is less than the second. Else, it will return 0.


// The actual ordered array.
typedef struct {
    type_t *array;
    uint32_t size;
    uint32_t maxSize;
    lessthan_predicate less_than;
} ordered_array_t;

#endif