// ordered_array.h - header file for ordered_array.c (ordered array stuff)
// This implementation is based on James Molloy's kernel development tutorials (http://www.jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html)

#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

// Includes
#include <libk_reduced/stdint.h>
#include <libk_reduced/assert.h> // Assert macro
#include <libk_reduced/ordered_array_t.h>

// Typedefs


// Functions
int8_t standard_lessthan_predicate(type_t a, type_t b); // A standard less-than predicate
ordered_array_t createOrderedArray(uint32_t maxSize, lessthan_predicate less_than); // Create an ordered array
ordered_array_t placeOrderedArray(void* addr, uint32_t maxSize, lessthan_predicate less_than); // Place an ordered array
void destroyOrderedArray(ordered_array_t* array); // Destroy an ordered array
void insertOrderedArray(type_t item, ordered_array_t* array); // Insert an item into an ordered array.
type_t lookupOrderedArray(uint32_t i, ordered_array_t* array); // Lookup an item at index i in ordered array array.
void removeOrderedArray(uint32_t i, ordered_array_t* array); // Remove an item at index i in ordered array array.

#endif
