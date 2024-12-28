/**
 * @file hexahedron/fs/vfs.c
 * @brief Virtual filesystem handler
 * 
 * @todo refcounts implemented
 * @todo locks
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/fs/vfs.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <kernel/mem/alloc.h>
#include <kernel/debug.h>

#include <structs/tree.h>
#include <structs/hashmap.h>
#include <structs/list.h>


/* Main VFS tree */
tree_t *vfs_tree = NULL;

/* Hashmap of filesystems (quick access) */
hashmap_t *vfs_filesystems = NULL;

/* Root node of the VFS */
fs_node_t *vfs_root = NULL;

/* Log method */
#define LOG(status, ...) dprintf_module(status, "VFS", __VA_ARGS__)

/* Old reduceOS implemented a CWD system, but that was just for the kernel CLI */

/**
 * @brief Standard POSIX open call
 * @param node The node to open
 * @param oflags The open flags
 */
void fs_open(fs_node_t *node, unsigned int oflags) {
    if (node->open) {
        return node->open(node, oflags);
    }
}

/**
 * @brief Standard POSIX close call
 * @param node The node to close
 */
void fs_close(fs_node_t *node) {
    if (node->close) {
        return node->close(node);
    }
}

/**
 * @brief Standard POSIX read call
 * @param node The node to read from
 * @param offset The offset to read at
 * @param size The amount of bytes to read
 * @param buffer The buffer to store the bytes in
 * @returns The amount of bytes read
 */
ssize_t fs_read(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    if (node->read) {
        return node->read(node, offset, size, buffer);
    }

    return 0;
}

/**
 * @brief Standard POSIX write call
 * @param node The node to write to
 * @param offset The offset to write at
 * @param size The amount of bytes to write
 * @param buffer The buffer of the bytes to write
 * @returns The amount of bytes written
 */
ssize_t fs_write(fs_node_t *node, off_t offset, size_t size, uint8_t *buffer) {
    if (node->write) {
        return node->write(node, offset, size, buffer);
    }

    return 0;
}

/**
 * @brief Read directory
 * @param node The node to read the directory of
 * @param index The index to read from
 * @returns A directory entry structure or NULL
 */
struct dirent *fs_readdir(fs_node_t *node, unsigned long index) {
    if (node->flags & VFS_DIRECTORY && node->readdir) {
        return node->readdir(node, index);
    }

    return NULL;
}

/**
 * @brief Find directory
 * @param node The node to find the path in
 * @param name The name of the file to look for
 * @returns The node of the file found or NULL
 */
fs_node_t *fs_finddir(fs_node_t *node, char *path) {
    if (node->flags & VFS_DIRECTORY && node->finddir) {
        return node->finddir(node, path);
    }

    return NULL;
}

/**
 * @brief Make directory
 * @param path The path of the directory
 * @param mode The mode of the directory created
 * @returns Error code
 */
int fs_mkdir(char *path, mode_t mode) { return -ENOTSUP; }

/**
 * @brief Unlink file
 * @param name The name of the file to unlink
 * @returns Error code
 */
int fs_unlink(char *name) { return -ENOTSUP; }



/**** VFS TREE FUNCTIONS ****/

char *vfs_canonicalizePath(char *cwd, char *addition);

/**
 * @brief Initialize the virtual filesystem with no root node.
 */
void vfs_init() {
    // Create the tree
    vfs_tree = tree_create("VFS");

    // Now create a blank root node.
    vfs_tree_node_t *root_node = kmalloc(sizeof(vfs_tree_node_t));
    root_node->fs_type = strdup("N/A");
    root_node->name = strdup("/");
    root_node->node = 0;
    tree_set_parent(vfs_tree, root_node);
    
    // Create the filesystem hashmap
    vfs_filesystems = hashmap_create("VFS filesystems", 10);

    LOG(INFO, "VFS initialized\n");
}


/**
 * @brief Canonicalize a path based off a CWD and an addition.
 * 
 * This basically will turn /home/blah (CWD) + ../other_directory/gk (addition) into
 * /home/other_directory/gk
 * 
 * @param cwd The current working directory
 * @param addition What to add to the current working directory
 */
char *vfs_canonicalizePath(char *cwd, char *addition) {
    char *canonicalize_path;

    // Is the first character of addition a slash? If so, that means the path we want
    // to canonicalize is just addition.
    if (*addition == '/') {
        canonicalize_path = strdup(addition);
        goto _canonicalize;
    }

    // We can just combine the paths now.
    if (cwd[strlen(cwd)-1] == '/') {
        // cwd ends in a slash (note that normally this shouldn't happen)
        canonicalize_path = kmalloc(strlen(cwd) + strlen(addition));
        sprintf(canonicalize_path, "%s%s", cwd, addition); // !!!: unsafe
    } else {
        // cwd does not end in a slash
        canonicalize_path = kmalloc(strlen(cwd) + strlen(addition) + 1);
        sprintf(canonicalize_path, "%s/%s", cwd, addition); // !!!: unsafe
    }


_canonicalize:
    // At this point canonicalize_path holds a raw path to parse.
    // Something like: /home/blah/../other_directory/gk
    // We'll pull a trick from old coding and parse it into a list, iterate each element and go.
    list_t *list = list_create("canonicalize list");

    dprintf(DEBUG, "canonicalize_path = %s\n", canonicalize_path);

    // Now add all the elements to the list, parsing each one.  
    size_t path_size = 0;  
    char *pch;
    char *save;
    pch = strtok_r(canonicalize_path, "/", &save);
    while (pch) {
        if (!strcmp(pch, "..")) {
            // pch = "..", go up one.
            node_t *node = list_pop(list);
            if (node) {
                kfree(node);
                kfree(node->value);
            }
        } else if (!strcmp(pch, ".")) {
            // Don't add it to the list, it's just a "."
        } else if (*pch) {
            // Normal path, add to list
            list_append(list, strdup(pch));
            path_size += strlen(pch) + 1; // +1 for the /
        }

        pch = strtok_r(NULL, "/", &save);
    }

    // Now we can free the previous path.
    kfree(canonicalize_path);

    // Allocate the new output path (add 1 for the extra \0)
    char *output = kmalloc(path_size + 1); // Return value
    memset(output, 0, path_size + 1);

    if (!path_size) {
        // The list was empty? No '/'s? Assume root directory
        LOG(WARN, "Empty path_size after canonicalization - assuming root directory.\n");
        kfree(output); // realloc is boring 
        output = strdup("/");
    } else {
        // Append each element together
        foreach(i, list) {
            // !!!: unsafe call to sprintf
            sprintf(output, "%s/%s", output, i->value);
        }
    }

    list_destroy(list, true); // Destroy the list & free contents
    return output;
}