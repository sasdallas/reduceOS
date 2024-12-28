/**
 * @file libpolyhedron/include/bits/dirent.h
 * @brief Directory entry
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

#ifndef _BITS_DIRENT_H
#define _BITS_DIRENT_H

/**** INCLUDES ****/
#include <stdint.h>

/**** TYPES ****/

typedef struct dirent {
    uint32_t d_ino;
    char d_name[256];
} dirent;

#endif

_End_C_Header