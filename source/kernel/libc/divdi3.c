// divdi3.c - replacement for the standard C file divdi3.c
// This code is BSD licensed, as it was taken from FreeBSD

#include "include/libc/divdi3.h"
#include "include/libc/quad.h"


// Divide two signed quads.
quad_t
__divdi3(a, b)
	quad_t a, b;
{
	u_quad_t ua, ub, uq;
	int neg;

	if (a < 0)
		ua = -(u_quad_t)a, neg = 1;
	else
		ua = a, neg = 0;
	if (b < 0)
		ub = -(u_quad_t)b, neg ^= 1;
	else
		ub = b;
	uq = __qdivrem(ua, ub, (u_quad_t *)0);
	return (neg ? -uq : uq);
}
