/**
 * @file hexahedron/kernel/config.c
 * @brief Configuration options for kernel.
 * 
 * Edit this file to change things like:
 * - Debugging port settings
 * - Version information
 * etc.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/config.h>


// This is part of vsnprintf() style formatting. TODO: Maybe not?
// Example: 1.0.0-i386-DEBUG
const char *__kernel_version_format = "%d.%d.%d-%s-%s";

// This file is deleted and rebuilt every build to update these.
const char *__kernel_build_date = __DATE__;
const char *__kernel_build_time = __TIME__;

// Versioning information
const int __kernel_version_major = 1;
const int __kernel_version_minor = 0;
const int __kernel_version_lower = 0;

// Codename
const char *__kernel_version_codename = "Phoenix";



/**** DEBUG SETTINGS ****/

#define DEBUG_COM_PORT 1        // This is the default COM port that Hexahedron will initialize and provide.
                                // WARNING: It is very much not recommended to use any port besides COM1.

#define DEBUG_BAUD_RATE 9600    // This is the default baud rate Hexahedron will set

const int __debug_com_port = DEBUG_COM_PORT;
const int __debug_baud_rate = DEBUG_BAUD_RATE;

/**** AUTO-GENERATED VERSIONING INFO ****/


// Configuration
#if defined(__KERNEL_DEBUG__)
const char *__kernel_build_configuration = "DEBUG";
#elif defined(__KERNEL_RELEASE__)
const char *__kernel_build_configuration = "RELEASE";
#else
const char *__kernel_build_configuration = "UNKNOWN";
#endif

// Architecture information
#ifdef __ARCH__
#define ARCH_STR(x) #x
const char *__kernel_architecture = ARCH_STR(__ARCH__);
#else
const char *__kernel_architecture = "unknownarch";
#endif

// Compiler information
#if (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
#define KERNEL_COMPILER_VERSION "GCC " __VERSION__;
#else
#define KERNEL_COMPILER_VERSION "Unknown Compiler"
#endif

const char *__kernel_compiler = KERNEL_COMPILER_VERSION;