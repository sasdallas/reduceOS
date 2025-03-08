/**
 * @file hexahedron/drivers/net/ethernet.c
 * @brief Layer 2 Ethernet handler
 * 
 * This is the layer-2 handler (OSI model) of the Hexahedron networking stack
 * 
 * NICs register themselves with the NIC manager and call @c ethernet_handle to handle
 * received packets or call @c ethernet_send to send packets.
 * 
 * Protocol handlers register themselves as EtherType handlers with @c ethernet_registerHandler
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/net/ethernet.h>
#include <kernel/drivers/net/nic.h>
#include <kernel/fs/vfs.h>
#include <kernel/debug.h>
#include <structs/hashmap.h>

/* EtherType handler list */
hashmap_t *ethertype_handler_map = NULL;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "NETWORK:ETH", __VA_ARGS__)

/**
 * @brief Register a new EtherType handler
 * @param ethertype The EtherType to register
 * @param handler The handler to register
 * @returns 0 on success
 */
int ethernet_registerHandler(uint16_t ethertype, ethertype_handler_t handler) {
    if (!ethertype_handler_map) ethertype_handler_map = hashmap_create_int("ethertype handlers", 20);

    if (hashmap_has(ethertype_handler_map, (void*)(uintptr_t)ethertype)) return 1;
    hashmap_set(ethertype_handler_map, (void*)(uintptr_t)ethertype, (void*)(uintptr_t)handler);
    return 0;
}

/**
 * @brief Unregister an EtherType handler
 * @param ethertype The EtherType to unregister
 */
int ethernet_unregisterHandler(uint16_t ethertype) {
    if (!hashmap_has(ethertype_handler_map, (void*)(uintptr_t)ethertype)) return 1;
    hashmap_remove(ethertype_handler_map, (void*)(uintptr_t)ethertype);
    return 0;
}

/**
 * @brief Handle a packet that was received by an Ethernet device
 * @param packet The ethernet packet that was received
 * @param nic_node The NIC that got the packet
 * @param size The size of the packet
 */
void ethernet_handle(ethernet_packet_t *packet, fs_node_t *nic_node, size_t size) {

}

/**
 * @brief Send a packet to an Ethernet device
 * @param nic_node The NIC to send the packet from
 * @param payload The payload to send
 * @param type The EtherType of the packet to send
 * @param dest_mac The destination MAC address of the packet
 * @param size The size of the packet
 */
void ethernet_send(fs_node_t *nic_node, void *payload, uint16_t type, uint8_t *dest_mac, size_t size) {

}