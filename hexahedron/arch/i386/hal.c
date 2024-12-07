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


/* General includes */
#include <time.h>
#include <string.h>

/* Kernel includes */
#include <kernel/arch/i386/arch.h>
#include <kernel/arch/i386/hal.h>
#include <kernel/hal.h>
#include <kernel/debug.h>
#include <kernel/config.h>
#include <kernel/debugger.h>
#include <kernel/mem/alloc.h>
#include <kernel/misc/term.h>

/* Generic drivers */
#include <kernel/drivers/serial.h>
#include <kernel/drivers/grubvid.h>
#include <kernel/drivers/video.h>
#include <kernel/drivers/font.h>

/* Architecture drivers */
#include <kernel/drivers/x86/serial.h>
#include <kernel/drivers/x86/clock.h>
#include <kernel/drivers/x86/pit.h>
#include <kernel/drivers/x86/acpica.h> // #ifdef ACPICA_ENABLED in this file

/* Root system descriptor pointer */
static uint64_t hal_rsdp = 0x0; 


/**
 * @brief Sets an RSDP if one was set
 */
void hal_setRSDP(uint64_t rsdp) {
    hal_rsdp = rsdp;
}

/**
 * @brief Returns a RSDP if one was found
 * 
 * You can call this method also to search for one (in EBDA/range)
 */
uint64_t hal_getRSDP() {
    if (hal_rsdp != 0x0) return hal_rsdp;
    
    // TODO: We can check EBDA/BDA but ACPICA (ACPI components) provides a method to check those for us.
    return 0x0;
}

/**
 * @brief Stage 1 startup - initializes logging, interrupts, clock, etc.
 */
static void hal_init_stage1() {
    // Initialize serial logging.
    if (serial_initialize()) {
        // didn't work... do something
    }

    // TODO: Is it best practice to do this in HAL?
    debug_setOutput(serial_print);
    arch_say_hello(1);

    // Initialize interrupts
    hal_initializeInterrupts();
    
    // Start the clock system
    clock_initialize();

    // Initialize the PIT
    pit_initialize();
}

/**
 * @brief Stage 2 startup - initializes debugger, ACPI, etc.
 */
static void hal_init_stage2() {

    // We need to reconfigure the serial ports and initialize the debugger.
    serial_setPort(serial_createPortData(__debug_output_com_port, __debug_output_baud_rate), 1);

    if (!__debugger_enabled) goto _no_debug;

    serial_port_t *port = serial_initializePort(__debugger_com_port, __debugger_baud_rate);
    if (!port) {
        dprintf(WARN, "Failed to initialize COM%i for debugging\n", __debugger_com_port);
        goto _no_debug;
    }

    serial_setPort(port, 0);
    if (debugger_initialize(port) != 1) {
        dprintf(WARN, "Debugger failed to initialize or connect.\n");
    }


_no_debug: ;

    // Next step is to initialize ACPICA. This is a bit hacky and hard to read.
#ifdef ACPICA_ENABLED
    // Initialize ACPICA
    int init_status = ACPICA_Initialize();
    if (init_status != 0) {
        dprintf(ERR, "ACPICA failed to initialize correctly - please see log messages.\n");
    }

    ACPICA_StartSMP();
#else
    // TODO: We can create a minified ACPI system that just handles SMP
    dprintf(WARN, "No ACPI subsystem is available to kernel\n");
#endif

    // Next, initialize video subsystem.
    video_init();

    video_driver_t *driver = grubvid_initialize(arch_get_generic_parameters());
    if (driver) {
        video_switchDriver(driver);
    }

    // Now, fonts - just do the backup one for now.
    font_init();

    // Terminal!
    int term = terminal_init(TERMINAL_DEFAULT_FG, TERMINAL_DEFAULT_BG);
    if (term != 0) {
        dprintf(WARN, "Terminal failed to initialize (return code %i)\n", term);
    }

    // Say hi again!
    arch_say_hello(0);


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


/* External functions given to kernel */
extern void halGetRegistersInternal(registers_t *regs);

/**
 * @brief Get registers from architecture
 * @returns Registers structure
 * @warning Untested
 */
struct _registers *hal_getRegisters() {
    registers_t *output = kmalloc(sizeof(registers_t));
    memset(output, 0, sizeof(registers_t));
    halGetRegistersInternal(output);
    return output;
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