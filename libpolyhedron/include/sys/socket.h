/**
 * @file libpolyhedron/include/sys/socket.h
 * @brief Socket address
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <sys/cheader.h>

_Begin_C_Header

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

/**** INCLUDES ****/
#include <stdint.h>
#include <stddef.h>

/**** DEFINITIONS ****/

#define SOCK_DGRAM          0       // Datagram socket
#define SOCK_RAW            1       // Raw protocol interface
#define SOCK_SEQPACKET      2       // Sequenced-packet stream
#define SOCK_STREAM         3       // Byte-stream socket

#define SOL_SOCKET          1

#define SO_ACCEPTCONN       0
#define SO_BROADCAST        1
#define SO_DEBUG            2
#define SO_DONTROUTE        3
#define SO_ERROR            4
#define SO_KEEPALIVE        5          
#define SO_LINGER           6
#define SO_OOBINLINE        7
#define SO_RCVBUF           8
#define SO_RCVLOWAT         9
#define SO_RCVTIMEO         10
#define SO_REUSEADDR        11
#define SO_SNDBUF           12
#define SO_SNDLOWAT         13
#define SO_SNDTIMEO         14
#define SO_TYPE             15

#define AF_INET             1       // IPv4
#define AF_INET6            2       // IPv6
#define AF_UNIX             3       // UNIX
#define AF_UNSPEC           4       // Unspecified

#define SHUT_RD             1
#define SHUT_RDWR           2
#define SHUT_WR             3

/**** TYPES ****/
typedef size_t socklen_t;
typedef unsigned int sa_family_t;

typedef struct sockaddr {
    sa_family_t     sa_family;      // Address family
    char            sa_data[];      // Socket address  
};


#endif

_End_C_Header