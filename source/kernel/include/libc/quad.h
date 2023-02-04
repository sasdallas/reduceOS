// quad.h - standard header file from the C standard

#ifndef QUAD_H
#define QUAD_H

// Includes
#include "include/libc/stdint.h" // Integer definitions


// Typedefs
typedef uint64_t u_quad_t;
typedef int64_t		quad_t;
typedef uint32_t	u_long;
typedef uint32_t 	qshift_t;


union uu {
	quad_t		q;		/* as a (signed) quad */
	quad_t		uq;		/* as an unsigned quad */
	int32_t		sl[2];		/* as two signed longs */
	uint32_t	ul[2];		/* as two unsigned longs */
};


// Macros and definitions
#define CHAR_BIT 8


#if BYTE_ORDER == LITTLE_ENDIAN
#define _QUAD_HIGHWORD	1
#define _QUAD_LOWWORD	0
#else
#define _QUAD_HIGHWORD	0
#define _QUAD_LOWWORD	1
#endif

#define H		_QUAD_HIGHWORD
#define L		_QUAD_LOWWORD
#define QUAD_BITS	(sizeof(quad_t) * CHAR_BIT)
#define LONG_BITS	(sizeof(long) * CHAR_BIT)
#define HALF_BITS	(sizeof(long) * CHAR_BIT / 2)


// Functions
u_quad_t __qdivrem(u_quad_t uq, u_quad_t vq, u_quad_t* arq);


#endif
