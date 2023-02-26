// =====================================================
// vfs.c - Virtual File System handler
// =====================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/vfs.h" // Main header file

fsNode_t *fs_root = 0; // Root of the filesystem

// Functions:

/* Most of these functions are really simple - just check if there's a callback in the node and call it! */

// readFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf) - Reads a file in a filesystem.
uint32_t readFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf) {
    // This is pretty simple, we just use the callbacks in each node.
    // Check if the node has a callback.
    if (node->read != 0) {
        return node->read(node, off, size, buf);
    } else {
        return 0; // It doesn't.
    }
}

// writeFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf) - Writes a file in a filesystem.
uint32_t writeFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf) {
    if (node->write != 0) {
        return node->write(node, off, size, buf);
    } else {
        return 0;
    }
}

// openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) - Opens a filesystem.
void openFilesystem(fsNode_t *node, uint8_t read, uint8_t write) {
    if (node->open != 0) {
        return node->open(node);
    }
}

// closeFilesystem(fsNode_t *node) - Closes a filesystem.
void closeFilesystem(fsNode_t *node) {
    if (node->close != 0)
        return node->close(node);
}

// Now it gets slightly more complicated.

// readDirectoryFilesystem(fsNode_t *node, uint32_t index) - Reads a directory in a filesystem.
struct dirent *readDirectoryFilesystem(fsNode_t *node, uint32_t index) {
    // Check if the node is a directory and if it has a callback.
    if ((node->flags & 0x7) == VFS_DIRECTORY && node->readdir != 0) {
        return node->readdir(node, index);
    } else { return 0; }
}


// findDirectoryFilesystem(fsNode_t *node, char *name) - Finds a directory in a filesystem.
fsNode_t *findDirectoryFilesystem(fsNode_t *node, char *name) {
    // Check if the node is a directory and if it has a callback.
    if ((node->flags & 0x7) == VFS_DIRECTORY && node->finddir != 0) {
        return node->finddir(node, name);
    } else { return 0; }
}