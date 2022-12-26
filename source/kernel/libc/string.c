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


// memset() - Set a buffer in memory to a given value.
// Three parameters - buffer, value, amount of times to set

void* memset(void *buf, int c, size_t n) {
    unsigned char* ubuf = buf;
    
    while (n--) { *ubuf++ = (unsigned char)c; }
    return buf;

}

// strlen() - Returns the length of a string(size_t)
// One parameter - str
int strlen(char *str) {
    int i = 0;
    while (*str++) {
        i++;
    }
    return i;
}


// itoa() - converts an integer to a string
// Three parameters - Integer, string buffer, integer base (1-16)
void itoa(int num, char *buffer, int base) {
    char bchars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}; // int -> char conversion thingy
    char tbuf[32]; // Temporary buffer
    int pos = 0;
    int opos = 0;
    int top = 0;

    if (num < 0) { // We need to do a little extra work if the value is negative.
        *buffer++ = '-';
        num *= -1;
    }

    if (num == 0 || base > 16) { // Don't even bother if the base is greater than 16 or the number is 0.
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    // First, get the character
    while (num != 0) {
        tbuf[pos] = bchars[num % base];
        pos++;
        num /= base;
    }

    top = pos--;

    for (opos = 0; opos < top; pos--,opos++) 
        buffer[opos] = tbuf[pos];
    
    buffer[opos] = 0;
}


// strcpy() - copies a string to a string.
// Two parameters - destination and source.
char *strcpy(char *dest, const char *src) {
    char *dest_p = dest;
    while (*dest++ = *src++); // Copy all the contents of src to destination.
    return dest_p;
}

// toupper() - turns a char uppercase
// One parameter - char.
char toupper(char c) {
    if ((c >= 'a') && (c <= 'z'))
        return (c - 32);
    return c;
}


// tolower() - turns a char lowercase
// One parameter - char.
char tolower(char c) {
    if ((c >= 'A') && (c <= 'Z'))
        return (c + 32);
    return c;
}

// isalpha() - returns if a char is in the alphabet
int isalpha(char ch) {
    return (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')));
}

// strcmp() - compares s1 to s2
// Two parameters - string #1 and #2.
int strcmp (const char* str1, char *str2) {
    int i = 0;

    while ((str1[i] == str2[i])) {
        if (str2[i++] == 0)
            return 0;
    }
    return 1;
}

