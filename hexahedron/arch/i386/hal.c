/**
 * @file hexahedron/arch/i386/hal.c
 * @brief Hardware abstraction layer for I386
 * 
 * This implements a hardware abstraction system for the architecture.
 * No specific calls should be made from generic. This includes interrupt handlers, port I/O, etc.
 * 
 * Generic HAL calls are implemented in kernel/hal.h
 * Architecture-specific HAL calls are implemented in kernel/arch/<architecture>/hal.h
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/arch/i386/arch.h>
#include <kernel/arch/i386/hal.h>
#include <kernel/hal.h>
#include <kernel/debug.h>

/* Generic drivers */
#include <kernel/drivers/serial.h>


/* Drivers. Find a better way to do this. */
#include <kernel/drivers/x86/serial.h>
#include <kernel/drivers/x86/clock.h>

#include <time.h>

/**
 * @brief Initialize the hardware abstraction layer
 * 
 * Initializes serial output, memory systems, and much more for I386.
 *
 * @todo A better driver interface is needed.
 */
void hal_init() {
    // Initialize serial logging.
    serial_initialize();

    // TODO: Is it best practice to do this in HAL?
    debug_setOutput(serial_writeCharacter);
    arch_say_hello();

    // Initialize interrupts
    hal_initializeInterrupts();
    
    // Start the clock system
    clock_initialize();

    time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  dprintf(INFO, "Current time: %s\n", asctime(timeinfo));
}

/* We do not need documentation comments for outportb/inportb/etc. */

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