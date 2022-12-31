// assert.h - contains the assertion macro

#ifndef ASSERT_H
#define ASSERT_H

// Includes
#include "include/panic.h"

// Macros

// ASSERT(b) - Makes sure b is a valid object before continuing.
#define ASSERT(b, caller, msg) ((b) ? (void)0 : panic("assert", caller, msg));

#endif