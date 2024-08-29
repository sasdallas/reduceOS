// ordered_array.c - Contains ordered array functions and definitions.
// Implementation credits in header file.

#include <libk_reduced/ordered_array.h> // Main header file

// Functions:

// standard_lessthan_predicate(type_t a, type_t b) - Returns whether a is less than b.
int8_t standard_lessthan_predicate(type_t a, type_t b) { return (a < b) ? 1 : 0; }

// createOrderedArray(uint32_t maxSize, lessthan_predicate less_than) - Creates and returns an ordered array
ordered_array_t createOrderedArray(uint32_t maxSize, lessthan_predicate less_than) {
    ordered_array_t retValue; // This is the array we will return
    retValue.array = (void*)kmalloc(maxSize * sizeof(type_t)); // Allocate space for the actual array
    memset(retValue.array, 0, maxSize * sizeof(type_t)); // Fill the array with zeroes

    // Finish filling in the remaining values.
    retValue.size = 0;
    retValue.maxSize = maxSize;
    retValue.less_than = less_than;

    // Return the ordered array
    return retValue;
}

// placeOrderedArray(void *addr, uint32_t maxSize, lessthan_predicate less_than) - Place an ordered array at address *addr (note: creates an ordered array)
ordered_array_t placeOrderedArray(void* addr, uint32_t maxSize, lessthan_predicate less_than) {
    ordered_array_t retValue;
    retValue.array = (type_t*)addr; // Set the array to the address given.
    memset(retValue.array, 0, maxSize * sizeof(type_t)); // Fill the array with zeroes

    // Finish filling in the remaining values.
    retValue.size = 0;
    retValue.maxSize = maxSize;
    retValue.less_than = less_than;

    // Return the ordered array.
    return retValue;
}

void destroyOrderedArray(ordered_array_t* array) {
    // Not available, since we need kfree, but heap uses this to setup kfree, so it doesn't really work.
}

// insertOrderedArray(type_t item, ordered_array_t *array) - Insert an item into array at a free index.
void insertOrderedArray(type_t item, ordered_array_t* array) {
    ASSERT(array->less_than, "insertOrderedArray", "Invalid array less than (is 0)"); // Assert the less_than (check if it's valid)

    uint32_t iterator = 0;
    while (iterator < array->size && array->less_than(array->array[iterator], item)) iterator++; // Get a free index in the ordered array.

    if (iterator == array->size) {
        // Easy! Just add it at the end of the array
        array->array[array->size++] = item;
    }
    else {
        // We have a little more work to do.
        type_t tmp = array->array[iterator];
        array->array[iterator] = item;
        while (iterator < array->size) {
            iterator++;
            type_t tmp2 = array->array[iterator];
            array->array[iterator] = tmp;
            tmp = tmp2;
        }
        array->size++;
    }
}

// lookupOrderedArray(uint32_t i, ordered_array_t *array) - Lookup an item in ordered array array at index i.
type_t lookupOrderedArray(uint32_t i, ordered_array_t* array) {
    ASSERT(i < array->size, "lookupOrderedArray", "Index too large."); // Assert i (check if it's valid)
    return array->array[i]; // Return the value
}

// removeOrderedArray(uint32_t i, ordered_array_t *array) - Remove an item from ordered array array at index i.
void removeOrderedArray(uint32_t i, ordered_array_t* array) {
    while (i < array->size) {
        // Shift values left and increment i.
        array->array[i] = array->array[i + 1];
        i++;
    }
    array->size--; // Decrement size
}
