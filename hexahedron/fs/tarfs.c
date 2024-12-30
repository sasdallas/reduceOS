/**
 * @file hexahedron/fs/tarfs.c
 * @brief USTAR archive filesystem, used for the initial ramdisk
 * 
 * @todo Need writing capabilities? We open argp in O_RDONLY
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
#include <kernel/mem/alloc.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

/**
 * @brief tarfs read method
 */
ssize_t tarfs_read(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    return 0;
}

/**
 * @brief tarfs write method
 */
ssize_t tarfs_write(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    return 0;
}

/**
 * @brief tarfs read directory method for root 
 */
struct dirent *tarfs_readdir_root(fs_node_t *node, unsigned long index) {
    return NULL;
}


/**
 * @brief Mount a tarfs filesystem
 * @param argp Expects a ramdev device to mount the filesystem based
 */
fs_node_t *tarfs_mount(char *argp, char *mountpoint) {
    fs_node_t *tar_file = kopen(argp, O_RDONLY);
    if (tar_file == NULL) {
        return NULL; // Failed to open
    }

    // Allocate a new filesystem node for us.
    fs_node_t *node = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    memset(node, 0, sizeof(fs_node_t));
    
    strncpy(node->name, mountpoint, 256);
    node->flags = VFS_DIRECTORY;
    node->permissions = 0770;
    node->dev = tar_file;

    // This is the root node, meaning that it doesn't have a read/write/open/close method.
    // Normal files will have their inodes set to an offset in the file
    
    return node;
}

/**
 * @brief Initialize the tarfs system
 */
void tarfs_init() {
    
}