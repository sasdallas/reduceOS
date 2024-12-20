/**
 * @file hexahedron/arch/stub/hal.c
 * @brief Stub file for HAL functions
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/x86_64/hal.h>
#include <kernel/hal.h>
#include <kernel/panic.h>

/**
 * @brief Register an interrupt handler
 * @param int_no Interrupt number
 * @param handler A handler. This should return 0 on success, anything else panics.
 *                It will take registers and extended registers as arguments.
 * @returns 0 on success, -EINVAL if handler is taken
 */
int hal_registerInterruptHandler(uint32_t int_no, interrupt_handler_t handler) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

/* PORT I/O FUNCTIONS */

void outportb(unsigned short port, unsigned char data) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

void outportw(unsigned short port, unsigned short data) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

void outportl(unsigned short port, unsigned long data) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

unsigned char inportb(unsigned short port) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

unsigned short inportw(unsigned short port) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}

unsigned long inportl(unsigned short port) {
    kernel_panic(UNSUPPORTED_FUNCTION_ERROR, "stub");
    __builtin_unreachable();
}