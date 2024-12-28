/**
 * @file hexahedron/include/kernel/fs/vfs.h
 * @brief VFS header file
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef KERNEL_FS_VFS_H
#define KERNEL_FS_VFS_H

/**** INCLUDES ****/
#include <stdint.h>
#include <stddef.h>
#include <bits/dirent.h>
#include <sys/types.h>
#include <structs/tree.h>


/**** DEFINITIONS ****/

// Type bitmasks
#define VFS_FILE            0x01
#define VFS_DIRECTORY       0x02
#define VFS_CHARDEVICE      0x04
#define VFS_BLOCKDEVICE     0x08
#define VFS_PIPE            0x10
#define VFS_SYMLINK         0x20
#define VFS_MOUNTPOINT      0x40
#define VFS_SOCKET          0x80


/**** TYPES ****/

// Node prototype
struct fs_node;

// These are the types of operations that can be performed on an inode.
// Sourced from the POSIX standard (tweaked to use fs_node rather than fd)
typedef void (*open_t)(struct fs_node*, unsigned int oflag); // oflag can be sourced from fcntl.h
typedef void (*close_t)(struct fs_node*);
typedef ssize_t (*read_t)(struct fs_node *, off_t, size_t, uint8_t*);
typedef ssize_t (*write_t)(struct fs_node *, off_t, size_t, uint8_t*);

typedef struct dirent* (*readdir_t)(struct fs_node *, unsigned long);
typedef struct fs_node* (*finddir_t)(struct fs_node *, char *);

typedef int (*mkdir_t)(struct fs_node *, char *, mode_t);
typedef int (*unlink_t)(struct fs_node *, char *);
typedef int (*readlink_t)(struct fs_node *, char *, size_t);
typedef int (*ioctl_t)(struct fs_node*, unsigned long, void *);
typedef int (*symlink_t)(struct fs_node*, char *, char *);


// Inode structure
typedef struct fs_node {
    char name[256];         // Node name (max of 256)
    mode_t permissions;     // Permissions mask
    uid_t uid;              // User ID
    gid_t gid;              // Group ID

    uint64_t flags;         // Node flags (e.g. VFS_SYMLINK)
    uint64_t inode;         // Device-specific, provides a way for the filesystem to idenrtify files
    uint64_t length;        // Size of file
    uint64_t impl;          // Implementation-defined number

    // Times
    time_t atime;           // Access timestamp (last access time)
    time_t mtime;           // Modified timestamp (last modification time)
    time_t ctime;           // Change timestamp (last metadata change time)

    // Functions
    read_t read;            // Read function
    write_t write;          // Write function
    open_t open;            // Open function
    close_t close;          // Close function
    readdir_t readdir;      // Readdir function
    finddir_t finddir;      // Finddir function
    // Create is not included - it can be done with open(O_CREAT) 
    mkdir_t mkdir;          // Mkdir function
    unlink_t unlink;        // Unlink function
    ioctl_t ioctl;          // I/O control function
    readlink_t readlink;    // Readlink function
    symlink_t symlink;      // Symlink function

    // Last file stuff
    struct fs_node *ptr;    // Used by mountpoints and symlinks
    int64_t refcount;       // Reference count on the file
    void *dev;              // Device structure
} fs_node_t;

// Hexahedron uses a mount callback system that works similar to interrupt handlers.
// Filesystems will register themselves with vfs_registerFilesystem() and provide a mount callback.
// Then when the user wants to mount something, all they have to do is call vfs_mountFilesystem which
// will try all possible filesystems on the drive.
// Note that vfs_mountFilesystem, vfs_mountFilesystemType, and vfs_mount are not the same

typedef fs_node* (*mount_callback)(char *argument, char *mountpoint); // Argument can be whatever, it's fs-specific.

typedef struct vfs_filesystem {
    char *name;             // Name of the filesystem
    mount_callback mount;   // Mount callback
} vfs_filesystem_t;

/**** FUNCTIONS ****/




#endif