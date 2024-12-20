/**
 * @file hexahedron/arch/stub/arch.c
 * @brief Stub file for main architecture
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/generic_mboot.h>
#include <kernel/panic.h>

int arch_current_cpu() {
    return 0;
}

void arch_panic_prepare() {

}

void arch_panic_finalize() {
    for (;;);
}

generic_parameters_t *arch_get_generic_parameters() {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}