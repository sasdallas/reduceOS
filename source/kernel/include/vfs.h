// vfs.h - Contains declarations of the virtual filesystem and the initial ramdisk.

#ifndef VFS_H
#define VFS_H

/* First, a quick explanation of what a VFS actually is:
   (wiki.osdev.org) A virtual filesystem is not an on-disk filesystem or a network filesystem. It's an abstraction that many OSes use to provide applications.
   A VFS is used to seperate the high-level interface to the FS from the low level interfaces that different implementations (like FAT, ext3, etc), may require. */

// Includes
#include "include/libc/stdint.h" // Integer declarations


// Definitions
#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02
#define VFS_CHARDEVICE 0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE 0x05
#define VFS_SYMLINK 0x06
#define VFS_MOUNTPOINT 0x08 // This one defines if the file is an active mountpoint




// Typedefs

// Filesystem node (prototype)
struct fsNode;

// Function prototypes (from the POSIX specification):
/* These prototypes define the type of callbacks that are called when the read/write/open/close methods are called */
typedef uint32_t (*read_t)(struct fsNode*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_t)(struct fsNode*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_t)(struct fsNode*);
typedef void (*close_t)(struct fsNode*);

// These aren't from the POSIX specification.
typedef struct dirent * (*readdir_t)(struct fsNode*, uint32_t);
typedef struct fsNode * (*finddir_t)(struct fsNode*, char *name);

// Actual typedef structures.
typedef struct {
    char name[128];     // FS Name (max of 128)
    uint32_t mask;      // Permissions mask
    uint32_t uid;       // Owning user
    uint32_t gid;       // Owning group
    uint32_t flags;     // Includes the node type.
    uint32_t inode;     // Device-specific, provides a way for a filesystem to identify files
    uint32_t length;    // Size of file.
    // uint32_t eof; - we can possible use for EOF instead of hacking up impl (and then use impl to define where we got the file from)
    uint32_t impl;      // Implementation defined number.
    read_t  read;
    write_t write;
    open_t open;
    close_t close;
    readdir_t readdir;
    finddir_t finddir;
    struct fsNode *ptr; // Used by mountpoints and symlinks.
} fsNode_t;

// Directory entry.
struct dirent {
    char name[128];     // Filename
    uint32_t ino;       // (required by POSIX) Inode number.
};

extern fsNode_t *fs_root; // Filesystem root

// Functions:
uint32_t readFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf); // Reads a file in a filesystem.
uint32_t writeFilesystem(fsNode_t *node, uint32_t off, uint32_t size, uint8_t *buf); // Writes a file in a filesystem.
void openFilesystem(fsNode_t *node, uint8_t read, uint8_t write); // Opens a filesystem.
void closeFilesystem(fsNode_t *node); // Closes a filesystem.
struct dirent *readDirectoryFilesystem(fsNode_t *node, uint32_t index); // Reads a directory in a filesystem.
fsNode_t *findDirectoryFilesystem(fsNode_t *node, char *name); // Finds a directory in a filesystem.
fsNode_t *openFile(const char *name); // Opens a file.


#endif