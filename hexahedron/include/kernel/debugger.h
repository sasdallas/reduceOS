/**
 * @file hexahedron/include/kernel/debugger.h
 * @brief Debugger header file
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_DEBUGGER_H
#define KERNEL_DEBUGGER_H

/**** INCLUDES ****/

#include <stdint.h>
#include <stddef.h>
#include <kernel/drivers/serial.h>

#include <structs/json-builder.h>
#include <structs/json.h>

/**** TYPES ****/

// Debug packets are just JSON objects
typedef json_value debug_packet_t;

/**** DEFINITIONS ****/

// Packet bytes
#define PACKET_START            0xAA
#define PACKET_END              0xBB

// Packet types
#define PACKET_TYPE_HELLO       0x01    // This is a hello packet

/**** FUNCTIONS ****/

/**
 * @brief Initialize the debugger. This will wait for a hello packet if configured.
 * @param port The serial port to use
 * @returns 1 on a debugger connecting, 0 on a debugger not connecting, and anything below zero is a bad input.
 */
int debugger_initialize(serial_port_t *port);

/**
 * @brief Send a packet to the debugger if connected
 * @param type The type of packet to send
 * @param object Any JSON object. This will be tied to a key labelled "data".
 * @returns 0 on success, non-zero is failure.
 */
int debugger_sendPacket(uint32_t type, json_value *object);

#endif