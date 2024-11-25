/**
 * @file libpolyhedron/include/stdlib.h
 * @brief Standard library header file
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <sys/cheader.h>

_Begin_C_Header

#ifndef _STDLIB_H
#define _STDLIB_H

/**** INCLUDES ****/
#include <stdint.h>

/**** FUNCTIONS ****/

unsigned long long int strtoull(const char*, char**, int);
unsigned long int strtoul(const char*, char**, int);
long int strtol(const char*, char**, int);
long long int strtoll(const char*, char**, int);
double strtod(const char*, char**);


#endif

_End_C_Header