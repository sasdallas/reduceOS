// tarfs.h - ustar filesystem driver

#ifndef TARFS_H
#define TARFS_H

// Includes
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <kernel/vfs.h>
#include <kernel/list.h>


// Structures
typedef struct {
    fsNode_t *device;
    unsigned int length;
} tarfs_t;

typedef struct {
    char filename[100];
    char mode[8];
    char ownerid[8];
    char groupid[8];

    char size[12];
    char modtime[12];

    char checksum[8];
    char type[1];
    char link[100];

    char ustar[6];
    char version[2];

    char owner[32];
    char group[32];

    char dev_major[8];
    char dev_minor[8];

    char prefix[155];
} ustar_t;

// Functions
int tarfs_register();
int tar_install();


#endif