// string.c - replacement for standard C string file. Contains certain useful functions like strlen.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#include "include/libc/string.h" // We need to include our header file.

// memcmp() - Comparing addresses/values in memory
// Three parameters - objects 1 and 2(const void*) and the amount of bytes to compare(size_t)

int memcmp(const void *s1, const void *s2, size_t n) {
    unsigned char *p1 = s1, *p2 = s2; // Changing them to types to avoid errors with the compiler
    
    if (s1 == s2) return 0; // Both pointers are pointing towards the same memory block. We don't need to check them.

    while (n > 0) {
        if (*p1 != *p2) return (*p1 > *p2) ? 1:-1; // If they are not the same, return either 1 or -1 depending on whether the mismatching character was greater or smaller than the other.
        n--; // Decrement n and increment p1 and p2 to move on to the next char
        p1++;
        p2++;
    }
}

// memcpy() - Copy a block of data from a source address to a destination address.
// Three parameters - source block(void*), destination block(void*), and amount of bytes to copy(size_t)

void* memcpy(void* __restrict source, const void* __restrict destination, size_t n) {
    // Typecast both addresses to char* and assign them to csrc and cdest.
    char *csrc = (char *)source, *cdest = (char *)destination;

    // Now copy the contents of source to destination.
    for (int i = 0; i < n; i++) { cdest[i] = csrc[i]; }
}



// memmove() - Move a block data from a source address to a destination address
// Three parameters - source block(void*), destination block(void*), and amount of bytes to move(size_t)

void* memmove(void* destination, const void* source, size_t n) {
    // First, we need to typecast destination to an unsigned char*, as well as typecast source to a constant unsigned char.
    unsigned char* dst = (unsigned char*) dst;
    const unsigned char* src = (const unsigned char*) source;

    // If the destination is smaller than the source, we can do a normal for loop with i < n.
    if (dst < src) {
        for (size_t i = 0; i < n; i++) 
            dst[i] = src[i];
        
    } else {
        // If that's not the case, the for loop can decrement i.
        for (size_t i = n; i != 0; i--)
            dst[i-1] = src[i-1]; // i-1 accounts for the indexes.
    }

    // Now just simply return destination (same type as function return, void*)
    return destination;
}


// strlen() - Returns the length of a string(size_t)
// One parameter - str(const char*)
size_t strlen(const char* str) {
    size_t stringLength = 0;
    while (*str++) stringLength++;
    return stringLength;
}

// itoa() - converts an integer to a string
// Three parameters - Integer, string buffer, integer base (1-10)
// Torn straight from the old reduceOS.
char* itoa(int num, char *buffer, int base) {
    char *p = buffer;
    char *p1, *p2;
    unsigned long ud = num;
    int divisor = 10;

    /* If %d is specified and D is minus, put ‘-’ in the head. */
    if (base == 'd' && num < 0) {
        *p++ = '-';
        buffer++;
        ud = -num;
    } else if (base == 'x')
        divisor = 16;

    /* Divide UD by DIVISOR until UD == 0. */
    do {
        int remainder = ud % divisor;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= divisor);

    /* Terminate buffer. */
    *p = 0;

    /* Reverse buffer. */
    p1 = buffer;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }

    

}