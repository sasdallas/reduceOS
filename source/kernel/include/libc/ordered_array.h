// ordered_array.h - header file for ordered_array.c (ordered array stuff)
// This implementation is based on James Molloy's kernel development tutorials (http://www.jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html)

#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/assert.h" // ASSERT macro

// Typedefs

typedef void* type_t; // This array is insertion sorted, meaning it always remains in a sorted state between calls.

typedef int8_t (*lessthan_predicate)(type_t, type_t); // A predicate should return non-zero if the first arg is less than the second. Else, it will return 0.

// The actual ordered array.
typedef struct {
    type_t *array;
    uint32_t size;
    uint32_t maxSize;
    lessthan_predicate less_than;
} ordered_array_t;


// Functions
int8_t standard_lessthan_predicate(type_t a, type_t b); // A standard less-than predicate
ordered_array_t createOrderedArray(uint32_t maxSize, lessthan_predicate less_than); // Create an ordered array
ordered_array_t placeOrderedArray(void *addr, uint32_t maxSize, lessthan_predicate less_than); // Place an ordered array
void destroyOrderedArray(ordered_array_t *array); // Destroy an ordered array
void insertOrderedArray(type_t item, ordered_array_t *array); // Insert an item into an ordered array.
type_t lookupOrderedArray(uint32_t i, ordered_array_t *array); // Lookup an item at index i in ordered array array.
void removeOrderedArray(uint32_t i, ordered_array_t *array); // Remove an item at index i in ordered array array.

#endif