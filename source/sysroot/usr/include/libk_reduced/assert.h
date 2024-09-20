// assert.h - contains the assertion macro


// I hate VS Code.
#ifndef __INTELLISENSE__
#if !defined __KERNEL__ && !defined __KERNMOD__
#error "This file cannot be used in userspace."
#endif
#endif


#ifndef ASSERT_H
#define ASSERT_H

// Includes
#include <kernel/panic.h>

// Macros

// ASSERT(b) - Makes sure b is a valid object before continuing.
#define ASSERT(b, caller, msg) ((b) ? (void)0 : panic("Assert", caller, msg));

#endif
