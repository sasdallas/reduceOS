// atomic.h - handles atomic math operations, used in coordination with the spinlock system.
// This file will need a refactor later, when we add multi-core support.
// This implementation of atomic math is based of eduOS by RWTH-OS.

#ifndef ATOMIC_H
#define ATOMIC_H

// Includes
#include "include/libc/stdint.h" // Integer declarations

// Typedefs and macros


// Standard atomic operations datatype
typedef struct { volatile int32_t counter; } atomic_int32_t;


// Macro for initialization of atomic_int32_t variables. Always use this macro to initialize.
#define ATOMIC_INIT(val) ((atomic_int32_t) { (val)})


// Inline functions.
inline static int32_t atomic_int32_test_and_set(atomic_int32_t *d, int32_t ret) {
    asm volatile ("xchgl %0, %1" : "=r"(ret) : "m"(d->counter), "0"(ret) : "memory");
    return ret;
}

inline static int32_t atomic_int32_add(atomic_int32_t *d, int32_t i)
{
	int32_t res = i;
	asm volatile("xaddl %0, %1" : "=r"(i) : "m"(d->counter), "0"(i) : "memory", "cc");
	return res+i;
}

inline static int32_t atomic_int32_sub(atomic_int32_t *d, int32_t i)
{
        return atomic_int32_add(d, -i);
}

inline static int32_t atomic_int32_inc(atomic_int32_t* d) {
	return atomic_int32_add(d, 1);
}

inline static int32_t atomic_int32_dec(atomic_int32_t* d) {
	return atomic_int32_add(d, -1);
}

inline static int32_t atomic_int32_read(atomic_int32_t *d) {
	return d->counter;
}

inline static void atomic_int32_set(atomic_int32_t *d, int32_t v) {
	atomic_int32_test_and_set(d, v);
}


#endif