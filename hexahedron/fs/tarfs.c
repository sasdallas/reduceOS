/**
 * @file hexahedron/fs/tarfs.c
 * @brief USTAR archive filesystem, used for the initial ramdisk
 * 
 * 
 * @copyright
 * This file is part of reduceOS, which is created by Samuel.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/fs/tarfs.h>
#include <kernel/fs/vfs.h>
#include <string.h>
#include <stdlib.h>


/**
 * @brief Mount a tarfs filesystem
 * @param argp Expects a ramdev device to mount the filesystem based
 */
fs_node_t *tarfs_mount(char *argp, char *mountpoint) {
    return NULL;
}

/**
 * @brief Initialize the tarfs system
 */
void tarfs_init() {

}