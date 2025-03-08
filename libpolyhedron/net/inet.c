/**
 * @file libpolyhedron/net/inet.c
 * @brief Byte-order stuff
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <arpa/inet.h>

uint32_t htonl(uint32_t hostlong) {
	return (
        (((hostlong) & 0xFF) << 24) | 
        (((hostlong) & 0xFF00) << 8) | 
        (((hostlong) & 0xFF0000) >> 8) | 
        (((hostlong) & 0xFF000000) >> 24)
    );
}

uint16_t htons(uint16_t hostshort) {
    return (
        (((hostshort) & 0xFF) << 8) |
        (((hostshort) & 0xFF00) >> 8)
    );
}

uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}