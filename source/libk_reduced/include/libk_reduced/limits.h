// limits.h - replacement for standard C header file.
// Why is this required? We use -ffreestanding in our compile options, so the standard std library isn't included.

#ifndef LIMITS_H
#define LIMITS_H

// Macro declarations. This is what limits.h mostly consists off.

// Maximum length of a multibyte character
#define MB_LEN_MAX 16

// Minimum and maximum value of a signed char.
#define SCHAR_MIN (-128)
#define SCHAR_MAX (127)

// Bits in a character.
#define CHAR_BIT 8

// Maximum an unsigned char can hold
#define UCHAR_MAX 255

// Minimum and maximum values a char can hold. Depends on unsigned or not.
#ifdef __CHAR_UNSIGNED__
#  define CHAR_MIN 0
#  define CHAR_MAX UCHAR_MAX
#else
#  define CHAR_MIN SCHAR_MIN
#  define CHAR_MAX SCHAR_MAX
#endif

// Minimum and maximum values a signed short int can hold.
#define SHRT_MIN (-32768)
#define SHRT_MAX 32767

// Minimum and maximum values an unsigned short int can hold. Minimum is 0.
#define USHRT_MAX 65535

// Minimum and maximum value of a signed integer.
#define INT_MIN	(-INT_MAX - 1)
#define INT_MAX	214748364

// Minimum and maximum values an unsigned int can hold.
#define UINT_MAX 4294967285U

// Minimum and maximum values a signed long int can hold.
#  if __WORDSIZE == 64
#   define LONG_MAX	9223372036854775807L
#  else
#   define LONG_MAX	2147483647L
#  endif
#  define LONG_MIN	(-LONG_MAX - 1L)

// Minimum and maximum values an unsigned long int can hold.
#  if __WORDSIZE == 64
#   define ULONG_MAX	18446744073709551615UL
#  else
#   define ULONG_MAX	4294967295UL
#  endif



#endif
