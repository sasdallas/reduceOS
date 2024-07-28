// ext2.h - EXT2 filesystem driver header

#ifndef EXT2_H
#define EXT2_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/ide_ata.h" // IDE/ATA driver

// Typedefs

// Extended superblock fields
typedef struct ext2_extended {
    uint32_t non_reserved_inode;        // First non-reserved inode in the system (version < 1.0 have it fixed as 11)
    uint16_t inode_struct_size;         // Size of each inode structure in bytes (version < 1.0 have it fixed as 128)
    uint16_t blockgroup_superblock;     // Block group that the superblock is part of
    uint32_t optional_features;         // See definitions for the bitmasks of this value
    uint32_t required_features;         // See definitions for the bitmasks of this value
    uint32_t readonly_features;         // Features that if not supported render the volume forced to be remounted R/O
    uint8_t filesystem_id[16];          // ID of the filesystem
    char volumeName[16];                // Null-terminated volume name
    char lastPath[64];                  // Null-terminated last path the volume was mounted to
    uint32_t compression_algorithm;     // Compression algorithm used
    uint8_t prealloc_files;             // Number of blocks to preallocate for the files
    uint8_t prealloc_directories;       // Number of blocks to preallocate for the directories
    uint16_t unused;                    // Unused
    uint8_t journal_id[16];             // Journal ID
    uint32_t journal_inode;             // Journal inode
    uint32_t journal_device;            // Journal device
    uint32_t orphan_head;               // Head of orphan inode list.

    char unused2[1024-236];
} __attribute__((packed)) ext2_extended_t;


// Superblock structure
typedef struct ext2_superblock {
    uint32_t total_inodes;              // Total amount of inodes
    uint32_t total_blocks;              // Total amount of blocks
    uint32_t superuser_reserved;        // Total amount of blocks reserved for the superuser
    uint32_t total_unallocated_blocks;  // Total number of unallocated blocks.
    uint32_t total_unallocated_inodes;  // Total number of unallocated inodes
    uint32_t superblock_number;         // Block number of the block containing the superblock
    uint32_t unshifted_block_size;      // Unshifted block size - shift 1024 to the left (or log2(block size) - 10)
    uint32_t unshifted_fragment_size;   // Unshifted fragment size - shift 1024 to the left (or log2(fragment size) - 10)
    uint32_t blockgroup_blocks;         // Number of blocks in each block group.
    uint32_t blockgroup_fragments;      // Number of fragments in each block group.
    uint32_t blockgroup_inodes;         // Number of inodes in each block group
    uint32_t last_mount_time;           // Last mount time in POSIX time
    uint32_t last_written_time;         // Last written time in POSIX time
    uint16_t mount_since_consistency;   // Number of times the volume has been mounted since the last consistency check.
    uint16_t mounts_before_check;       // Number of mounts allowed before next consistency check.
    uint16_t ext2_signature;            // Signature of EXT2 (0xEF53)
    uint16_t filesystem_state;          // State of the filesystem
    uint16_t error_method;              // What to do upon error
    uint16_t minor_version;             // Minor portion of version
    uint32_t last_consistency_check;    // Last consistency check in POSIX time
    uint32_t interval_check;            // Interval between consistency checks
    uint32_t creator_os_id;             // Operating system ID of creator (see os_id)
    uint32_t major_version;             // Major portion of version
    uint16_t reserved_userID;           // User ID that can use reserved blocks
    uint16_t reserved_groupID;          // Group ID that can use reserved blocks
    ext2_extended_t extension;          // Extended superblock (mainly for ext3 and ext4, but has some good values)
} __attribute__((packed)) ext2_superblock_t;


// Block group descriptor
typedef struct ext2_bgd {
    uint32_t block_usage_bitmap;        // Block address of block usage bitmap
    uint32_t inode_usage_bitmap;        // Block address of inode usage bitmap
    uint32_t inode_table;               // Starting block address of inode table
    uint16_t unallocated_blocks;        // Number of unallocated blocks in groups
    uint16_t unallocated_inodes;        // Number of unallocated inodes in groups
    uint16_t directories;               // Number of directories in groups
    uint16_t pad;
    uint8_t reserved[12];
} __attribute__((packed)) ext2_bgd_t;

// Inode
typedef struct ext2_inode {
    uint16_t permissions;                       // Types and permissions
    uint16_t uid;                               // User ID
    uint32_t size;                              // Lower 32 bits of size in bytes
    uint32_t last_access;                       // Last access time (in POSIX time)
    uint32_t creation_time;                     // Creation time (in POSIX time)
    uint32_t last_modification;                 // Last modification time (in POSIX time)
    uint32_t deletion_time;                     // Deletion time (in POSIX time)
    uint16_t gid;                               // Group ID
    uint16_t hard_links;                        // Count of dirents to this inode (0 = data blocks are unallocated)
    uint32_t disk_sectors;                      // SECTORS in use by this inode
    uint32_t flags;                             // Flags (see definitions)
    uint32_t os_specific1;                      // OS-specific value
    uint32_t blocks[15];                        // DBPs and IBPs (12 direct blocks + 3 indirect blocks)
    uint32_t generation;                        // Generation # (primarily NFS)
    uint32_t extended_attr_block;               // Extended attribute block ONLY IF version >= 1, else reserved.
    union {
        uint32_t dir_acl;                       // Used if version is not 0 and if inode is directory.
        uint32_t size_high;                     // Used if version is not 0 and if inode is file.
    };
    uint32_t fragment_block_addr;               // Fragment block address
    uint8_t os_specific2[12];                   // OS-specific value
} __attribute__((packed)) ext2_inode_t;

// Directory entry
typedef struct ext2_dirent {
    uint32_t inode;                             // Inode # for the dirent
    uint16_t entry_size;                        // Size of this entry
    uint8_t name_length;                        // Least-significant 8 bytes of the name length
    uint8_t type;                               // Type indicator (only if dirents have file type byte is set)
    char name[];                                // Directory name
} __attribute__((packed)) ext2_dirent_t;


// Cache structure
typedef struct ext2_cache {
    uint32_t block;
    uint32_t times;
    uint8_t dirty;
    char *block_data;
} __attribute__((packed)) ext2_cache_t;

// Enums
typedef enum {
    LINUX = 0,
    GNU_HURD = 1,
    MASIX = 2,
    FREEBSD = 3,
    OTHER = 4
} os_id;

typedef enum {
    IGNORE = 1,                         // Ignore the error
    REMOUNT_RO = 2,                     // Remount as R/O
    PANIC = 3                           // Kernel panic
} error_method;

typedef enum {
    CLEAN = 1,
    ERROR = 2
} fs_state;



// Structure to help organize and manage the ext2 filesystem
typedef struct ext2 {
    fsNode_t *drive;                    // ext2 drive
    ext2_superblock_t *superblock;      // Superblock structure
    uint32_t block_size;                // Block size
    uint32_t blocks_per_group;          // Blocks per group
    uint32_t inodes_per_group;          // Inodes per group
    uint32_t total_groups;              // Total groups
    uint32_t bgd_blocks;                // Block Group Descriptor blocks
    ext2_bgd_t *bgd_list;               // Block Group Descriptor list
} ext2_t;

// Definitions

// General
#define EXT2_SIGNATURE 0xEF53
#define EXT2_DIRECT_BLOCKS 12  
#define EXT2_ROOT_INODE_NUMBER 2


// Filesystem states
#define EXT2_FS_CLEAN 1
#define EXT2_FS_ERROR 2

// Optional feature flags (bitmask)
#define EXT2_PREALLOCATE_BLOCKS 0x0001       // Preallocate some amount of blocks to a directory when creating a new one
#define EXT2_AFS_SERVER_INODES 0x0002        // AFS server inodes exist
#define EXT2_JOURNAL_EXISTS 0x0004           // Filesystem has a journal (ext3)
#define EXT2_INODES_EXTENDED 0x0008          // Inodes can have extended attributes
#define EXT2_FS_RESIZE 0x0010                // Filesystem can resize itself for larger partitions
#define EXT2_DIRS_USE_HASH_INDEX 0x0020      // Directories use hash index

// Required feature flags (bitmask)
#define EXT2_COMPRESSION_USED 0x0001         // Compression is used
#define EXT2_DIRECTORIES_TYPEFIELD 0x0002    // Directory entries contain a type field
#define EXT2_FS_REPLAY_JOURNAL 0x0004        // Filesystem needs to replay its journal
#define EXT2_FS_JOURNAL_DEVICE 0x0008        // Filesystem uses a journal device

// Read-only feature flags
#define EXT2_SPARSE_SUPERBLKS_GROUPD 0x0001  // Sparse superblocks and group descriptor tables
#define EXT2_FILESIZE_64BIT 0x0002           // Filesystem uses a 64-bit file size
#define EXT2_DIR_BINARYTREE 0x0004           // Directory contents are stroed as a binary tree

// Inode types
#define EXT2_INODE_FIFO 0x1000               // FIFO
#define EXT2_INODE_CHARDEV 0x2000            // Character device
#define EXT2_INODE_DIRECTORY 0x4000          // Directory
#define EXT2_INODE_BLKDEVICE 0x6000          // Block device
#define EXT2_INODE_FILE 0x8000               // Regular old file
#define EXT2_INODE_SYMLINK 0xA000            // Symbolic link
#define EXT2_INODE_SOCKET 0xC000             // Unix socket

// Inode permissions
#define EXT2_PERM_OX 0x001                  // Other execute
#define EXT2_PERM_OW 0x002                  // Other write
#define EXT2_PERM_OR 0x004                  // Other read
#define EXT2_PERM_GX 0x008                  // Group execute
#define EXT2_PERM_GW 0x010                  // Group write
#define EXT2_PERM_GR 0x020                  // Group read
#define EXT2_PERM_UX 0x040                  // User execute
#define EXT2_PERM_UW 0x080                  // User write
#define EXT2_PERM_UR 0x100                  // User read
#define EXT2_PERM_STICKY 0x200              // Sticky bit
#define EXT2_PERM_SETGID 0x400              // Set group ID
#define EXT2_PERM_SETUID 0x800              // Set user ID

// Inode flags (only ones needed)
#define EXT2_INODE_SYNCUPD 0x00000008       // Syncronous updates - new data written immediately to disk
#define EXT2_INODE_IMMUTABLE 0x00000010     // Immutable file
#define EXT2_INODE_APPEND 0x00000020        // Append only
#define EXT2_INODE_NODUMP 0x00000040        // File is not included in the 'dump' command
#define EXT2_INODE_NOUPDACCESS 0x00000080   // Last accessed time should not be updated
#define EXT2_INODE_HASHIDX 0x00010000       // Hash indexed directory
#define EXT2_INODE_AFS 0x00020000           // AFS directory
#define EXT2_INODE_JOURNALDATA 0x00040000   // Journal file data

// Macros (for using BGDs)
#define BLOCKBIT(n)  (bg_buffer[((n) >> 3)] & (1 << (((n) % 8))))
#define BLOCKBYTE(n) (bg_buffer[((n) >> 3)])
#define SETBIT(n)    (1 << (((n) % 8)))


// Functions
int ext2_readBlock(ext2_t *fs, uint32_t block, uint8_t *buf); // Read a block from the device
int ext2_writeBlock(ext2_t *fs, uint32_t block, uint8_t *buf); // Write a block to the device
uint8_t *ext2_readInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock); // Reads a block in the specified inode
void ext2_writeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock, uint8_t *buffer); // Writes a block in the specified inode
uint32_t ext2_allocateBlock(ext2_t *fs); // Allocate a block from the ext2 block bitmaps
void ext2_freeBlock(ext2_t *fs, uint32_t block); // Frees a block from the ext2 block bitmaps
void ext2_rewriteBGDs(ext2_t *fs); // Rewrite the block group descriptors
ext2_superblock_t *ext2_readSuperblock(fsNode_t *device); // Reads and returns the superblock for a drive
int ext2_writeSuperBlock(ext2_t *fs); // Rewrites the superblock according to the superblock currently on the strutcure
int ext2_readInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index); // Given an inode number, find the inode on the disk and read it
void ext2_writeInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index); // Write an inode metadata at an index
uint32_t ext2_getDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock); // Gets the actual index of inodeBlock on the disk
uint32_t ext2_readInodeFiledata(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer); // Read the actual file data referenced from the inode
int ext2_readInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index); // Given an inode number, find the inode on the disk and read it.
void ext2_allocateInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block); // Allocate an inode block
void ext2_freeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block); // Free an inode block
uint32_t ext2_allocateInode(ext2_t *fs); // Allocate an inode from the inode bitmap
void ext2_freeInode(ext2_t *fs, uint32_t inode); // Frees an inode from the inode bitmap
fsNode_t *ext2_finddir(fsNode_t *node, char *name); // Returns NULL if file exists, and the node for the file if it does



#endif