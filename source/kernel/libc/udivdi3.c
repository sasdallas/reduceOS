// udivdi3.c - replacement for the standard C file udivdi3.c
// This code is BSD-licensed, as it is taken from FreeBSD.

#include "include/libc/udivdi3.h"



// Divide two unsigned quads.
u_quad_t __udivdi3(a, b)
u_quad_t a, b;
{

	return (__qdivrem(a, b, (u_quad_t *) 0));
}