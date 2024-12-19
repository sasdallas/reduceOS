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
    "KERNEL_BAD_ARGUMENT_ERROR",
    "OUT_OF_MEMORY",
    "IRQ_HANDLER_FAILED",
    "CPU_EXCEPTION_UNHANDLED",
    "UNSUPPORTED_FUNCTION_ERROR",
    "ACPI_SYSTEM_ERROR",
    "ASSERTION_FAILED"
};

char *kernel_panic_messages[KERNEL_STOP_CODES] = {
    "A trap was triggered to debug the kernel, but no debugger was connected.\n",
    "A fault has occurred in the memory management subsystem during a call.\n",
    "A bad argument was passed to a critical function. This is (unless specified) a bug in the kernel - please contact the developers.\n",
    "The system has run out of memory. Try closing applications or adjusting your pagefile.\n",
    "An IRQ handler did not return a success value. This could be caused by an external driver or an internal kernel driver.\n",
    "A CPU exception in the kernel was not handled correctly.",
    "An unsupported kernel function was called. This as a bug in the kernel - please contact the developers.\n",
    "Your computer is not compliant with ACPI specifications, or is not compatible with the ACPICA library.\n",
    "An assertion within the kernel failed.\n"
};