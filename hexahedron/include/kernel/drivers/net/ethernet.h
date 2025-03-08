/**
 * @file hexahedron/include/kernel/drivers/net/ethernet.h
 * @brief Ethernet layer (layer 2)
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_NET_ETHERNET_H
#define DRIVERS_NET_ETHERNET_H

/**** INCLUDES ****/
#include <kernel/drivers/net/nic.h>
#include <kernel/fs/vfs.h>
#include <stdint.h>
#include <stddef.h>

/**** TYPES ****/

/**
 * @brief Ethernet packet structure
 */
typedef struct ethernet_packet {
    uint8_t destination_mac[6];         // Target MAC address
    uint8_t source_mac[6];              // Source MAC adress
    uint16_t ethertype;                 // EtherType field (register your handlers with ethernet_registerHandler)
    uint8_t payload[];                  // Payload
} __attribute__((packed)) __attribute__((aligned(2))) ethernet_packet_t;

/**
 * @brief EtherType hander
 * @param frame The frame that was received (does not include the ethernet header)
 * @param nic_node The NIC that got the packet 
 * @param size The size of the packet
 */
typedef int (*ethertype_handler_t)(void *frame, fs_node_t *nic_node, size_t size);

/**** FUNCTIONS ****/

/**
 * @brief Register a new EtherType handler
 * @param ethertype The EtherType to register
 * @param handler The handler to register
 * @returns 0 on success
 */
int ethernet_registerHandler(uint16_t ethertype, ethertype_handler_t handler);

/**
 * @brief Unregister an EtherType handler
 * @param ethertype The EtherType to unregister
 */
int ethernet_unregisterHandler(uint16_t ethertype);

/**
 * @brief Handle a packet that was received by an Ethernet device
 * @param packet The ethernet packet that was received
 * @param nic_node The NIC that got the packet
 * @param size The size of the packet
 */
void ethernet_handle(ethernet_packet_t *packet, fs_node_t *nic_node, size_t size);

/**
 * @brief Send a packet to an Ethernet device
 * @param nic_node The NIC to send the packet from
 * @param payload The payload to send
 * @param type The EtherType of the packet to send
 * @param dest_mac The destination MAC address of the packet
 * @param size The size of the packet
 */
void ethernet_send(fs_node_t *nic_node, void *payload, uint16_t type, uint8_t *dest_mac, size_t size);



#endif