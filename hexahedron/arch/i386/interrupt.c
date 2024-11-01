/**
 * @file hexahedron/arch/i386/interrupt.c
 * @brief Interrupt handler & register
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/hal.h>
#include <kernel/arch/i386/interrupt.h>
#include <kernel/hal.h>

#include <kernel/debug.h>

#include <stdint.h>

extern void hal_installGDT();

/**
 * @brief Initialize HAL interrupts (IDT, GDT, TSS, etc.)
 */
void hal_initializeInterrupts() {
    // Start the GDT
    hal_installGDT();
}