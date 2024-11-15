/**
 * @file hexahedron/kernel/panic.c
 * @brief Kernel panic handler (generic)
 * 
 * Hexahedron uses two panic systems:
 * - Generic errors can be handled using bugcodes, such as MEMORY_MANAGEMENT_ERROR
 * - Nongeneric errors can call kernel_panic_prepare() and kernel_panic_finalize() to handle PRINT OUT the error in their own way
 *      * IMPORTANT: If you need to handle the error in your own way, use arch_panic_prepare() and arch_panic_finalize()
 * 
 * These stop codes are supported by a string lookup table, that @todo should be moved to a separate file.
 * (ReactOS uses .mc files, dunno about Linux - that's for localization however)
 * 
 * This file exposes 2 functions
 * - kernel_panic(BUGCODE, MODULE)
 * - kernel_panic_extended(BUGCODE, MODULE, FORMAT, ...)
 * 
 * kernel_panic() will take in a bugcode and a module (e.g. "vfs" or "mem") and use a generic string table to
 * print basic information
 * 
 * kernel_panic_extended() will take in a bugcode and a module, but will also take in va_args().
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/panic.h>
#include <kernel/debug.h>
#include <kernel/arch/arch.h>

#include <stdint.h>
#include <stdio.h>

/**
 * @brief xvas_callback
 */
static int kernel_panic_putchar(void *user, char ch) {
    dprintf(NOHEADER, "%c", ch);
    return 0;
}

/**
 * @brief Immediately panic and stop the kernel.
 * 
 * @param bugcode Takes in a kernel bugcode, e.g. KERNEL_DEBUG_TRAP
 * @param module The module the fault occurred in, e.g. "vfs"
 * @param format String to print out (this is va_args, you may use formatting)
 */
void kernel_panic_extended(uint32_t bugcode, char *module, char *format, ...) {
    if ((int)bugcode >= __kernel_stop_codes) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, module, "*** kernel_panic_extended() received an invalid bugcode (0x%x)\n", bugcode);
        // Doesn't return
    }

    // Prepare for the panic
    arch_panic_prepare();

    // Start by printing out debug messages
    dprintf(NOHEADER, "\033[1;31m\n\nFATAL: Kernel panic detected!\n\033[0;31m");
    dprintf(NOHEADER, "Hexahedron has experienced a critical fault that cannot be resolved\n");
    dprintf(NOHEADER, "Please start an issue on GitHub if you believe this to be a bug.\n");
    dprintf(NOHEADER, "Apologies for any inconveniences caused by this error.\n\n");
    
    dprintf(NOHEADER, "*** STOP: %s (module \'%s\')\n", kernel_bugcode_strings[bugcode], module);

    va_list ap;
    va_start(ap, format);
    xvasprintf(kernel_panic_putchar, NULL, format, ap);
    va_end(ap);

    // Print out a generic message
    dprintf(NOHEADER, "\n%s", kernel_panic_messages[bugcode]);

    // Finish the panic
    dprintf(NOHEADER, "\nThe kernel will now permanently halt. Connect a debugger for more information.\n");
    arch_panic_finalize();
}

/**
 * @brief Immediately panic and stop the kernel.
 * @param bugcode Takes in a kernel bugcode, e.g. KERNEL_DEBUG_TRAP
 * @param module The module the fault occurred in, e.g. "vfs"
 */
void kernel_panic(uint32_t bugcode, char *module) {
    if ((int)bugcode >= __kernel_stop_codes) {
        kernel_panic_extended(KERNEL_BAD_ARGUMENT_ERROR, module, "*** kernel_panic() received an invalid bugcode (0x%x)\n", bugcode);
        // Doesn't return
    }
    
    // Prepare for the panic
    arch_panic_prepare();

    // Start by printing out debug messages
    dprintf(NOHEADER, "\033[1;31m\n\nFATAL: Kernel panic detected!\n\033[0;31m");
    dprintf(NOHEADER, "Hexahedron has experienced a critical fault that cannot be resolved\n");
    dprintf(NOHEADER, "Please start an issue on GitHub if you believe this to be a bug.\n");
    dprintf(NOHEADER, "Apologies for any inconveniences caused by this error.\n\n");
    dprintf(NOHEADER, "*** STOP: %s (module \'%s\')\n", kernel_bugcode_strings[bugcode], module);
    dprintf(NOHEADER, "*** %s", kernel_panic_messages[bugcode]);
    
    // Finish the panic
    dprintf(NOHEADER, "\nThe kernel will now permanently halt. Connect a debugger for more information.\n");
    arch_panic_finalize();
}

/**
 * @brief Prepare the system to enter a panic state
 * 
 * @param bugcode Optional bugcode to display. Leave as NULL to not use (note: generic string will not be printed)
 */
void kernel_panic_prepare(uint32_t bugcode) {
    arch_panic_prepare();

    dprintf(NOHEADER, "\033[1;31m\n\nFATAL: Kernel panic detected!\n\033[0;31m");
    dprintf(NOHEADER, "Hexahedron has experienced a critical fault that cannot be resolved\n");
    dprintf(NOHEADER, "Please start an issue on GitHub if you believe this to be a bug.\n");
    dprintf(NOHEADER, "Apologies for any inconveniences caused by this error.\n\n");

    if (bugcode) {
        dprintf(NOHEADER, "\033[1;31m*** STOP: %s\n", kernel_bugcode_strings[bugcode]);
    }
}

/**
 * @brief Finalize the panic state
 */
void kernel_panic_finalize() {
    dprintf(NOHEADER, "\nThe kernel will now permanently halt. Connect a debugger for more information.\n");
    arch_panic_finalize();
}