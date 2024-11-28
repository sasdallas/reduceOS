/**
 * @file hexahedron/debug/debugger.c
 * @brief Provides the main interface of the Hexahedron debugger.
 * 
 * The debugger and the kernel communicate via JSON objects.
 * On startup, the kernel will wait for a hello packet from the debugger, then start
 * communication from there.
 * 
 * Packets are constructed like so:
 * - Newline
 * - Initial @c PACKET_START byte
 * - Length of the packets (int)
 * - JSON string
 * - Final @c PACKET_END byte
 * - Newline
 * 
 * The JSON itself isn't very important (you can provide your own JSON
 * fields) - the main important thing is the pointer to the packet's structure.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/debugger.h>
#include <kernel/generic_mboot.h>
#include <kernel/arch/arch.h>
#include <kernel/misc/spinlock.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/pmm.h>
#include <kernel/debug.h>
#include <kernel/config.h>
#include <kernel/misc/pool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ARCH_I386__)
#include <kernel/arch/i386/hal.h>
#endif

/**** VARIABLES ****/

/* Log method */
#define LOG(status, format, ...) dprintf_module(status, "DEBUGGER", format, ## __VA_ARGS__)

/* Debugger interface port */
serial_port_t *debugger_port = NULL;

/* Debugger lock */ 
spinlock_t *debug_lock = NULL;

/**** FUNCTIONS ****/

/**
 * @brief Handshake with debugger
 * @returns Non-zero on successful handshake
 */
int debugger_handshake() {
    // Create a packet containing kernel information and system information
    json_value *data = json_object_new(10);
    
    // Create version info (major, minor, lower)
    json_value *version_info = json_object_new(4);
    json_object_push(version_info, "major", json_integer_new(__kernel_version_major));
    json_object_push(version_info, "minor", json_integer_new(__kernel_version_minor));
    json_object_push(version_info, "lower", json_integer_new(__kernel_version_lower));
    json_object_push(version_info, "codename", json_string_new(__kernel_version_codename));
    json_object_push(data, "version", version_info);

    // Create build information (date/time/debug/architecture/etc.)
    json_value *build_info = json_object_new(6);
    json_object_push(build_info, "date", json_string_new(__kernel_build_date));
    json_object_push(build_info, "time", json_string_new(__kernel_build_time));
    json_object_push(build_info, "conf", json_string_new(__kernel_build_configuration));
    json_object_push(build_info, "arch", json_string_new(__kernel_architecture));
    json_object_push(build_info, "compiler", json_string_new(__kernel_compiler));
    json_object_push(data, "build", build_info);


    // Create heap and allocator information (location/usage/name)

extern uintptr_t mem_kernelHeap;
extern uintptr_t mem_identityMapCacheSize;
extern pool_t *mem_mapPool;

    json_value *heap_info = json_object_new(7);
    char temp_string[128];

    snprintf(temp_string, 16, "%x", mem_kernelHeap);
    json_object_push(heap_info, "heap_location", json_string_new(temp_string));
    json_object_push(heap_info, "im_pool_usage", json_integer_new(mem_mapPool->allocated));
    json_object_push(heap_info, "im_cache_size", json_integer_new(mem_identityMapCacheSize));

    json_object_push(heap_info, "total_pmm_blocks", json_integer_new(pmm_getMaximumBlocks()));
    json_object_push(heap_info, "free_pmm_blocks", json_integer_new(pmm_getFreeBlocks()));
    json_object_push(heap_info, "pmm_block_size", json_integer_new(PMM_BLOCK_SIZE));
    
    allocator_info_t *allocInfo = alloc_getInfo();
    snprintf(temp_string, 128, "%s %i.%i", allocInfo->name, allocInfo->version_major, allocInfo->version_minor);
    json_object_push(heap_info, "alloc_name", json_string_new(temp_string));

    json_object_push(data, "heap", heap_info);

    // Create system information
    generic_parameters_t *parameters = arch_get_generic_parameters();
    json_value *sys_info = json_object_new(4);
    json_object_push(sys_info, "memory_size", json_integer_new(parameters->mem_size));
    json_object_push(sys_info, "bootloader", json_string_new(parameters->bootloader_name));
    json_object_push(sys_info, "cmdline", json_string_new(parameters->kernel_cmdline));

    // Create memory map information (subset of sysinfo)
    json_value *mmap_info = json_array_new(0);

    generic_mmap_desc_t *desc = parameters->mmap_start;
    while (desc) {
        json_value *desc_value = json_object_new(4);
        snprintf(temp_string, 16, "%016llX", desc->address);
        json_object_push(desc_value, "address", json_string_new(temp_string));
        snprintf(temp_string, 16, "%016llX", desc->length);
        json_object_push(desc_value, "length", json_string_new(temp_string));
        json_object_push(desc_value, "type", json_integer_new(desc->type));
        json_array_push(mmap_info, desc_value);

        desc = desc->next;
    }

    json_object_push(sys_info, "mmap", mmap_info);
    json_object_push(data, "sysinfo", sys_info);

    // Send it off!
    int packet = debugger_sendPacket(PACKET_TYPE_HELLO, data);
    if (packet != 0) goto _cleanup;

    // Response!
    debug_packet_t *response = debugger_receivePacket(__debugger_wait_time);
    if (!response) goto _cleanup;

    json_builder_free(data);
    return 1;

_cleanup:
    json_builder_free(data);
    return 0;
}

#if defined(__ARCH_I386__)
/**
 * @brief Interrupt 3 breakpoint handler
 */
int debugger_breakpointHandler(uint32_t exception_number, registers_t *regs, extended_registers_t *extended) {
    json_value *breakpoint_data = json_object_new(3);
    json_object_push(breakpoint_data, "type", json_integer_new(BREAKPOINT_TYPE_INT3));

    // Encode the registers structure as bytes
    json_value *registers_encoded = json_array_new(sizeof(regs));
    uint8_t *ptr = (uint8_t*)regs;
    for (size_t i = 0; i < sizeof(registers_t); i++) {
        json_array_push(registers_encoded, json_integer_new((int)ptr[i]));
    }

    json_object_push(breakpoint_data, "registers", registers_encoded);

    // Now do the same for the extended registers structure
    json_value *ext_registers_encoded = json_array_new(sizeof(extended));
    ptr = (uint8_t*)extended;
    for (size_t i = 0; i < sizeof(extended_registers_t); i++) {
        json_array_push(ext_registers_encoded, json_integer_new((int)ptr[i]));
    }

    json_object_push(breakpoint_data, "extended_registers", ext_registers_encoded);

    // Send the packet!
    LOG(DEBUG, "Entering breakpoint state (INT3 triggered)\n");
    debugger_sendPacket(PACKET_TYPE_BREAKPOINT, breakpoint_data);

    // Enter a permanent loop waiting for more packets
    for (;;); // jk 
}
#endif

/**
 * @brief Initialize the debugger. This will wait for a hello packet if configured.
 * @param port The serial port to use
 * @returns 1 on a debugger connecting, 0 on a debugger not connecting, and anything below zero is a bad input.
 */
int debugger_initialize(serial_port_t *port) {
    if (!port || !port->read || !port->write) return -EINVAL;
    debugger_port = port;
    debug_lock = spinlock_create("debugger_lock");

    LOG(INFO, "Trying to initialize the debugger...\n");


    int handshake = debugger_handshake();
    if (!handshake) goto _no_debug;

#if defined(__ARCH_I386__)
    // Register an exception handler for INT3
    hal_registerExceptionHandler(0x03, debugger_breakpointHandler);
    asm volatile ("int $0x03");
#endif

    return 1;

_no_debug:
    debugger_port = NULL; // Null these out to prevent any stupidity
    return 0;
}

