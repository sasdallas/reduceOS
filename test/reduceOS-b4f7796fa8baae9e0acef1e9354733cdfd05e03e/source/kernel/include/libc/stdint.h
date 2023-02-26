// stdint.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef STDINT_H
#define STDINT_H

#define NULL 0

// Exact-width integer types

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

// Minimum-width integer types.

typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int_least16_t;
typedef unsigned short uint_least16_t;
typedef int int_least32_t;
typedef unsigned uint_least32_t;
typedef long long int_least64_t;
typedef unsigned long long uint_least64_t;



#endif