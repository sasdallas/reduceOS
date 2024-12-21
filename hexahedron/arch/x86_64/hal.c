/**
 * @file hexahedron/arch/x86_64/hal.c
 * @brief x86_64 hardware abstraction layer
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

// General
#include <stdint.h>

// Kernel includes
#include <kernel/arch/x86_64/arch.h>
#include <kernel/hal.h>
#include <kernel/debug.h>
#include <kernel/panic.h>

// Drivers (x86)
#include <kernel/drivers/x86/serial.h>
#include <kernel/drivers/x86/clock.h>


/**
 * @brief Stage 1 startup - initializes logging, interrupts, clock, etc.
 */
static void hal_init_stage1() {
    // Initialize serial driver
    if (serial_initialize() == 0) {
        // Setup debug output
        debug_setOutput(serial_print);
    }    

    // Say hi!
    arch_say_hello(1);

    // Initialize clock driver
    clock_initialize();

    // Initialize interrupts
    hal_initializeInterrupts();
}



/**
 * @brief Stage 2 startup - initializes debugger, ACPI, etc.
 */
static void hal_init_stage2() {

}


/**
 * @brief Initialize the hardware abstraction layer
 * 
 * Initializes serial output, memory systems, and much more for I386.
 *
 * @param stage What stage of HAL initialization should be performed.
 *              Specify HAL_STAGE_1 for initial startup, and specify HAL_STAGE_2 for post-memory initialization startup.
 * @todo A better driver interface is needed.
 */
void hal_init(int stage) {
    if (stage == HAL_STAGE_1) {
        hal_init_stage1();
    } else if (stage == HAL_STAGE_2) {
        hal_init_stage2();
    }
}


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
    __asm__ __volatile__("outb %b[Data], %w[Port]" :: [Port] "Nd" (port), [Data] "a" (data));
}

void outportw(unsigned short port, unsigned short data) {
    __asm__ __volatile__("outw %w[Data], %w[Port]" :: [Port] "Nd" (port), [Data] "a" (data));
}

void outportl(unsigned short port, unsigned long data) {
    __asm__ __volatile__("outl %k[Data], %w[Port]" :: [Port] "Nd"(port), [Data] "a" (data));
}

unsigned char inportb(unsigned short port) {
    unsigned char returnValue;
    __asm__ __volatile__("inb %w[Port]" : "=a"(returnValue) : [Port] "Nd" (port));
    return returnValue;
}

unsigned short inportw(unsigned short port) {
    unsigned short returnValue;
    __asm__ __volatile__("inw %w[Port]" : "=a"(returnValue) : [Port] "Nd" (port));
    return returnValue;
}

unsigned long inportl(unsigned short port) {
    unsigned long returnValue;
    __asm__ __volatile__("inl %w[Port]" : "=a"(returnValue) : [Port] "Nd" (port));
    return returnValue;
}