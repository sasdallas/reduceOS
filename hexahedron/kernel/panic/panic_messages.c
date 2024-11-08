/**
 * @file hexahedron/kernel/panic/panic_messages.c
 * @brief Kernel panic messages 
 * 
 * Contains a list of panic messages
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/panic.h>


int __kernel_stop_codes = KERNEL_STOP_CODES; // Used to keep this as the only needed-recomp file

char *kernel_bugcode_strings[KERNEL_STOP_CODES] = {
    "KERNEL_DEBUG_TRAP",
    "MEMORY_MANAGEMENT_ERROR",
    "KERNEL_BAD_ARGUMENT_ERROR"
};

char *kernel_panic_messages[KERNEL_STOP_CODES] = {
    "This is a trap used to debug the kernel. Be careful when using debug!.\n",
    "A generic fault has occurred in the memory management subsystem.\nThis is most likely a bug in the kernel - please contact for assistance.\n",
    "A bad argument was passed to a critical function.\n"
};