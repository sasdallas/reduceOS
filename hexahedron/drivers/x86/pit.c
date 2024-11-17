/**
 * @file hexahedron/drivers/x86/pit.c
 * @brief Programmable interval timer driver
 * 
 * @note    This interfaces with the global clock driver, but it functions only as something to update the ticks with.
 *          Because CMOS has no way to be updated every so often, this is done instead.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/x86/pit.h>
#include <kernel/drivers/clock.h>
#include <kernel/arch/i386/hal.h> // !! BAD HAL PROBLEM

#include <kernel/debug.h>


static uint64_t pit_ticks = 0; // TODO: Make clock_update conform better so we can remove this variable.

/**
 * @brief Change the PIT timer phase.
 * @warning Don't touch unless you know what you're doing.
 */
void pit_setTimerPhase(long hz) {
    long divisor = PIT_SCALE / hz; // Only can change the divisor.

    outportb(PIT_MODE, PIT_RATE_GENERATOR | PIT_LOBYTE_HIBYTE);
    outportb(PIT_CHANNEL_A, divisor & 0xFF);
    outportb(PIT_CHANNEL_A, (divisor >> 8) & 0xFF);
}

/**
 * @brief IRQ handler
 */
int pit_irqHandler(uint32_t exception_index, uint32_t int_number, registers_t *regs, extended_registers_t *regs_extended) {
    pit_ticks++;
    clock_update(pit_ticks);

    return 0;
}

/**
 * @brief Initialize PIT
 */
void pit_initialize() {
    // On default use 100 Hz for divisor
    pit_setTimerPhase(100);

    // Register handler
    hal_registerInterruptHandler(PIT_IRQ, pit_irqHandler);

    dprintf(INFO, "Programmable interval timer initialized\n");
}