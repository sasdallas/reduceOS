/**
 * @file crt0_main.c
 * @brief This is the C part of the file that is run on execution of a C program (CRT0).
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 *
 * @copyright 
 * The Newlib C library and other components such as libgloss are not governed by the reduceOS BSD license.
 * The licenses for the software contained in this library can be found in "COPYING.NEWLIB"
 * For more information, please contact the maintainers of the package.
 *
 */

#include <fcntl.h>

extern void exit(int code);
extern int main(int argc, char *argv[]);

extern char **environ;

void crt0_main(int envc, int argc, int args) {

    // Copy envc out to env
    int *env_start = &args;
    char *env[envc];
    for (int i = 0; i < envc; i++) {
        env[i] = (*(env_start + i));
    }

    environ = env;

    // We need to copy args out to argv
    int *args_start = env_start + envc + 1;
    char *argv[argc];
    for (int i = 0; i < argc; i++) {
        argv[i] = (*(args_start + i));
    }

    int ex = main(argc, argv);
    _exit(ex);
}