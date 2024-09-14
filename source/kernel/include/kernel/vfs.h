// vfs.h - Contains declarations of the virtual filesystem and the initial ramdisk.

#ifndef VFS_H
#define VFS_H

/* First, a quick explanation of what a VFS actually is:
   (wiki.osdev.org) A virtual filesystem is not an on-disk filesystem or a network filesystem. It's an abstraction that many OSes use to provide applications.
   A VFS is used to seperate the high-level interface to the FS from the low level interfaces that different implementations (like FAT, ext3, etc), may require. */

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <kernel/tree.h> // Mountpoint tree
#include <kernel/hashmap.h> // Mount hashmap


// Definitions (TODO: Shouldn't these be bitmasks?)
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


// Offset
typedef uint64_t off_t;

// Function prototypes (from the POSIX specification):
/* These prototypes define the type of callbacks that are called when the read/write/open/close methods are called */
typedef uint32_t (*read_t)(struct fsNode*, off_t, uint32_t, uint8_t*);
typedef uint32_t (*write_t)(struct fsNode*, off_t, uint32_t, uint8_t*);
typedef void (*open_t)(struct fsNode*);
typedef void (*close_t)(struct fsNode*);

// These aren't from the POSIX specification.
typedef struct dirent * (*readdir_t)(struct fsNode*, uint32_t);
typedef struct fsNode * (*finddir_t)(struct fsNode*, char *name);
typedef int (*create_t) (struct fsNode *, char *name, uint16_t permission);
typedef int (*mkdir_t) (struct fsNode *, char *name, uint16_t permission);
typedef int (*unlink_t) (struct fsNode *, char *name);
typedef int (*ioctl_t) (struct fsNode *, unsigned long request, void *argp);

// Actual typedef structures.
typedef struct fsNode {
    char name[128];     // FS Name (max of 128)
    uint32_t mask;      // Permissions mask
    uint32_t uid;       // Owning user
    uint32_t gid;       // Owning group
    uint32_t flags;     // Includes the node type.
    uint32_t inode;     // Device-specific, provides a way for a filesystem to identify files
    uint32_t length;    // Size of file.
    uint32_t impl;      // Implementation defined number.
    uint32_t *impl_struct; // Implementation structure.
    read_t read;        // Read function
    write_t write;      // Write function
    open_t open;        // Open function
    close_t close;      // Close function
    readdir_t readdir;  // Readdir function
    finddir_t finddir;  // Finddir function
    create_t create;    // Create function
    mkdir_t mkdir;      // Make directory function.
    unlink_t unlink;    // Unlink function (deletion)
    ioctl_t ioctl;      // I/O control function
    struct fsNode *ptr; // Used by mountpoints and symlinks.
    int references;     // Child processes (and other cores if SMP is added) will sometimes hold access to the same node. This stores the reference count.
    void *device;       // Pointer to the device needed (unused mostly, most drivers will use impl_struct but you can use this)
} fsNode_t;

// Hashmap callback
typedef fsNode_t * (*vfs_mountCallback)(const char * arg, const char * mount_point);


// Directory entry.
struct dirent {
    char name[256];     // Filename
    uint32_t ino;       // (required by POSIX) Inode number.
};

// Entry for the filesystem tree
typedef struct vfsEntry {
    char name[20];
    fsNode_t *file; 
    char *device;
    char *fs_type;
} vfsEntry_t;



extern fsNode_t *fs_root; // Filesystem root

// Functions:
uint32_t readFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf); // Reads a file in a filesystem.
uint32_t writeFilesystem(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf); // Writes a file in a filesystem.
void openFilesystem(fsNode_t *node, uint8_t read, uint8_t write); // Opens a filesystem.
void closeFilesystem(fsNode_t *node); // Closes a filesystem.
struct dirent *readDirectoryFilesystem(fsNode_t *node, uint32_t index); // Reads a directory in a filesystem.
fsNode_t *findDirectoryFilesystem(fsNode_t *node, char *name); // Finds a directory in a filesystem.
fsNode_t *openFile(const char *name); // Opens a file.
fsNode_t *getRootFilesystem(); // Returns root filesystem.
void *vfsMount(char *path, fsNode_t *localRoot); // Mount a filesystem to the specified path
void vfsInit(); // Initialize the VFS
fsNode_t *open_file(const char *filename, unsigned int flags); // Opens a file using the VFS current working directory
void change_cwd(const char *newdir); // Changes the current working directory
char *get_cwd(); // Returns the current working directory
fsNode_t *open_file_recursive(const char *filename, uint64_t flags, uint64_t symlink_depth, char *relative); // Opens a file (but recursive and does all the work
fsNode_t *vfs_getMountpoint(char *path, unsigned int path_depth, char **outpath, unsigned int *outdepth); // Gets the mountpoint of something at path
int vfs_mountType(const char *type, const char *arg, const char *mountpoint); // Calls the mount handler for the filesystem driver for type
int vfs_registerFilesystem(const char *name, vfs_mountCallback callback); // Registers a filesystem mount callback that will be called when needed
void vfs_mapDirectory(const char *c); // Maps a directory in the virtual filesystem
char *vfs_canonicalizePath(const char *cwd, const char *input); // Canonicalizes a path
void debug_print_vfs_tree(bool printout);
fsNode_t *cloneFilesystemNode(fsNode_t *node);
void vfs_lock(fsNode_t *node);
int createFilesystem(char *name, uint16_t mode);
int ioctlFilesystem(fsNode_t *node, unsigned long request, void *argp);
int mkdirFilesystem(char *name, uint16_t mode);

#endif
