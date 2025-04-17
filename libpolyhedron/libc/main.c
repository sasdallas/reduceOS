/**
 * @file libpolyhedron/libc/main.c
 * @brief libc startup code
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <unistd.h>

// TODO: Init array support, mark as constructor - got a lot to do

void __libc_main(int argc, char **argv, char **envp, int (*main)(int, char**)) {
    exit(main(argc, argv));
}