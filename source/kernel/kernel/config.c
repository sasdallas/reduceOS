/**
 * @file source/kernel/kernel/config.c
 * @brief Holds configuration data for the kernel. Provides debugging information and more
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#define _KERNEL_CONFIGURATION_FILE
#include <kernel/CONFIG.h>

char *__kernel_build_number = BUILD_NUMBER;
char *__kernel_build_date = BUILD_DATE;
char *__kernel_version = VERSION;
char *__kernel_codename = CODENAME;
char *__kernel_configuration = BUILD_CONFIGURATION;


#ifdef ARCH_I386
char *__kernel_architecture = "i386";
#elif defined(ARCH_X86_64)
char *__kernel_architecture = "x86_64";
#else 
char *__kernel_architecture = "unknown - how are you running this";
#endif



#if (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
# define COMPILER_VERSION "GCC " __VERSION__
#elif (defined(__clang__))
# define COMPILER_VERSION "Clang " __clang_version__
#else
# define COMPILER_VERSION "UNKNOWN-COMPILER"
#endif

char *__kernel_compiler = COMPILER_VERSION;