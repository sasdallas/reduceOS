/**
 * @file hexahedron/debug/debugger.c
 * @brief Provides the main interface of the Hexahedron debugger.
 * 
 * The debugger and the kernel communicate via JSON objects.
 * On startup, the kernel will wait for a hello packet from the debugger, then start
 * communication from there.
 * 
 * Packets are constructed like so:
 * - Initial @c PACKET_START byte
 * - JSON string
 * - Final @c PACKET_END byte
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
#include <kernel/misc/spinlock.h>
#include <kernel/mem/alloc.h>
#include <kernel/debug.h>
#include <kernel/config.h>
#include <errno.h>

/* Log method */
#define LOG(status, format, ...) dprintf_module(status, "DEBUGGER", format, ## __VA_ARGS__)

/* Debugger interface port */
static serial_port_t *debugger_port = NULL;

/* Debugger lock */
static spinlock_t *debug_lock = NULL;

/**
 * @brief Send a packet, internally
 * @param packet The packet to send
 */
static void debugger_sendPacketInternal(debug_packet_t *packet) {
    if (!packet) return;
    spinlock_acquire(debug_lock);

    char *buffer = kmalloc(json_measure(packet));
    json_serialize(buffer, packet);

    // Print the packet
    serial_print((void*)debugger_port, PACKET_START);
    serial_printf("%s", buffer);
    serial_print((void*)debugger_port, PACKET_END);

    spinlock_release(debug_lock);
}

/**
 * @brief Initialize the debugger. This will wait for a hello packet if configured.
 * @param port The serial port to use
 * @returns 1 on a debugger connecting, 0 on a debugger not connecting, and anything below zero is a bad input.
 */
int debugger_initialize(serial_port_t *port) {
    if (!port || !port->read || !port->write) return -EINVAL;
    debugger_port = port;
    debug_lock = spinlock_create("debugger_lock");

    // Send a hello packet.
    
    json_value *object = json_object_new(3);
    debugger_sendPacket(PACKET_TYPE_HELLO, object);

    return 0;
}


/**
 * @brief Send a packet to the debugger if connected
 * @param type The type of packet to send
 * @param object Any JSON object. This will be tied to a key labelled "data".
 * @returns 0 on success, non-zero is failure.
 */
int debugger_sendPacket(uint32_t type, json_value *object) {
    if (!object) return 1;

    // Create the packet structure
    debug_packet_t *packet = json_object_new(5);
    if (!packet) {
        LOG(WARN, "json_object_new returned NULL\n");
        return 1;
    }

    // Get time first
    time_t rawtime;
    time(&rawtime);
    struct tm *tm = localtime(&rawtime);

    // Now we should pack some information
    json_object_push(packet, "type", json_integer_new(type)); // "type": type
    json_object_push(packet, "time", json_string_new(asctime(tm))); // "time": time

    // Who cares
    debugger_sendPacketInternal(packet);
    return 0;
}