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


// GCC doesn't provide a good macro for (u)intptr_t so we check if __PTRDIFF_TYPE__ is defined and use that, else do something different I guess
#if defined(__PTRDIFF_TYPE__)
typedef signed __PTRDIFF_TYPE__ intptr_t;
typedef unsigned __PTRDIFF_TYPE__ uintptr_t;

#define INTPTR_MAX PTRDIFF_MAX
#define INTPTR_MIN PTRDIFF_MIN

#ifdef __UINTPTR_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#else
#define UINTPTR_MAX (2UL * PTRDIFF_MAX + 1)
#endif
#else

// Fallback to hardcoded values
typedef signed long intptr_t;
typedef unsigned long uintptr_t;

#define INTPTR_MAX __STDINT_EXP(LONG_MAX)
#define INTPTR_MIN (-__STDINT_EXP(LONG_MAX) - 1)
#define UINTPTR_MAX (__STDINT_EXP(LONG_MAX) * 2UL + 1)

#endif


#endif