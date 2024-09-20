// ============================================================
// ext2.c - EXT2 filesystem driver for reduceOS
// ============================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// This code is also heavily based off of klange's ToaruOS ext2 module. Link: https://github.com/klange/toaruos/blob/master/modules/ext2.c#L1215

#include <kernel/ext2.h>    // Main header file
#include <kernel/ide_ata.h> // IDE/ATA driver
#include <kernel/vfs.h>
#include <kernel/mem.h>

#pragma GCC diagnostic ignored "-Wempty-body" // Don't want to mess with the func that uses this

// External variables
extern ideDevice_t ideDevices[4];

/* BLOCK FUNCTIONS */

// ext2_readBlock(ext2_t *fs, uint32_t block, uint8_t *buf) - Read a block from the device
int ext2_readBlock(ext2_t* fs, uint32_t block, uint8_t* buf) {
    uint64_t offset = (uint64_t)(fs->block_size) * block;
    int ret = fs->drive->read(fs->drive, offset, fs->block_size, buf);
    return ret;
}

// ext2_writeBlock(ext2_t *fs, uint32_t block, uint8_t *buf) - Write a block to the device
int ext2_writeBlock(ext2_t* fs, uint32_t block, uint8_t* buf) {
    uint64_t offset = (uint64_t)(fs->block_size) * block;
    return fs->drive->write(fs->drive, offset, fs->block_size, buf);
}

// ext2_readInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock) - Reads a block in the specified inode
int ext2_readInodeBlock(ext2_t* fs, ext2_inode_t* inode, uint32_t inodeBlock, uint8_t* buffer) {
    if (inodeBlock >= inode->disk_sectors / (fs->block_size / 512)) {
        memset(buffer, 0x0, fs->block_size);
        //serialPrintf("ext2_readInodeBlock: Tried to read an invalid block. Asked for %i (0-indexed) but inode only has %i\n", inodeBlock, inode->disk_sectors / (fs->block_size / 512));
        return -1;
    }

    // Get the disk block number and read it in
    unsigned int diskBlock = ext2_getDiskBlockNumber(fs, inode, inodeBlock);
    ext2_readBlock(fs, diskBlock, buffer);

    return 0;
}

// ext2_writeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock, uint8_t *buffer) - Writes a block in the specified inode
uint32_t ext2_writeInodeBlock(ext2_t* fs, ext2_inode_t* inode, uint32_t inodeNumber, uint32_t block, uint8_t* buffer) {
    if (block >= (inode->disk_sectors / (fs->block_size / 512))) {
        // (debugme)
    }

    char* empty = NULL;

    // Allocate the inode blocks
    while (block >= (inode->disk_sectors / (fs->block_size / 512))) {
        ext2_allocateInodeBlock(fs, inode, inodeNumber, inode->disk_sectors / (fs->block_size / 512));
        ext2_refreshInode(fs, inode, inodeNumber);

        //ext2_inode_t *inode2 = ext2_readInodeMetadata(fs, inodeNumber);
    }

    if (empty) kfree(empty); // ???

    uint32_t realBlock = ext2_getDiskBlockNumber(fs, inode, block);
    // serialPrintf("ext2_writeInodeBlock: Writing virtual block %i for inode %i maps to real block %i\n", block, inodeNumber, realBlock);

    ext2_writeBlock(fs, realBlock, buffer);

    return realBlock;
}

// ext2_getDiskBlockNumber(ext2_t *this, ext2_inode_t *inode, uint32_t iblock) - Gets the actual index of inodeBlock on the disk
uint32_t ext2_getDiskBlockNumber(ext2_t* this, ext2_inode_t* inode, uint32_t iblock) {
    // This function is stupidly complex, and you know what they say: "steal from the best, invent the rest."
    // This function and ext2_setDiskBlockNumber, as well as a few others are sourced from klange's osdev project with a decent amount of modifications and bugfixes

    unsigned int p = this->block_size / 4;

    /* We're going to do some crazy math in a bit... */
    unsigned int a, b, c, d, e, f, g;

    uint8_t* tmp;

    if (iblock < EXT2_DIRECT_BLOCKS) {
        return inode->blocks[iblock];
    } else if (iblock < EXT2_DIRECT_BLOCKS + p) {
        /* XXX what if inode->block[EXT2_DIRECT_BLOCKS] isn't set? */
        tmp = kmalloc(this->block_size);
        ext2_readBlock(this, inode->blocks[EXT2_DIRECT_BLOCKS], (uint8_t*)tmp);

        unsigned int out = ((uint32_t*)tmp)[iblock - EXT2_DIRECT_BLOCKS];
        kfree(tmp);
        return out;
    } else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p) {
        a = iblock - EXT2_DIRECT_BLOCKS;
        b = a - p;
        c = b / p;
        d = b - c * p;

        tmp = kmalloc(this->block_size);
        ext2_readBlock(this, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (uint8_t*)tmp);

        uint32_t nblock = ((uint32_t*)tmp)[c];
        ext2_readBlock(this, nblock, (uint8_t*)tmp);

        unsigned int out = ((uint32_t*)tmp)[d];
        kfree(tmp);
        return out;
    } else if (iblock < EXT2_DIRECT_BLOCKS + p + p * p + p) {
        a = iblock - EXT2_DIRECT_BLOCKS;
        b = a - p;
        c = b - p * p;
        d = c / (p * p);
        e = c - d * p * p;
        f = e / p;
        g = e - f * p;

        tmp = kmalloc(this->block_size);
        ext2_readBlock(this, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (uint8_t*)tmp);

        uint32_t nblock = ((uint32_t*)tmp)[d];
        ext2_readBlock(this, nblock, (uint8_t*)tmp);

        nblock = ((uint32_t*)tmp)[f];
        ext2_readBlock(this, nblock, (uint8_t*)tmp);

        unsigned int out = ((uint32_t*)tmp)[g];
        kfree(tmp);
        return out;
    }

    return 0;
}

// ext2_setDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t inodeBlock, uint32_t diskBlock) - Sets a disk block number
void ext2_setDiskBlockNumber(ext2_t* fs, ext2_inode_t* inode, uint32_t index, uint32_t inodeBlock, uint32_t diskBlock) {
    uint32_t p = fs->block_size / 4;
    int a, b, c, d, e, f, g;
    int iblock = inodeBlock;
    uint32_t* tmp = kmalloc(fs->block_size);

    a = iblock - EXT2_DIRECT_BLOCKS;
    if (a <= 0) {
        inode->blocks[inodeBlock] = diskBlock;
        goto done;
    }

    b = a - p;
    if (b <= 0) {
        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS]), inode, index, NULL, 0))
            ;
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (uint8_t*)tmp);
        ((uint32_t*)tmp)[a] = diskBlock;
        ext2_writeBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (uint8_t*)tmp);
        tmp[a] = diskBlock;
        goto done;
    }

    c = b - p * p;
    if (c <= 0) {
        c = b / p;
        d = b - c * p;
        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS + 1]), inode, index, NULL, 0))
            ;
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[c]), inode, index, (uint8_t*)tmp,
                                             inode->blocks[EXT2_DIRECT_BLOCKS + 1]))
            ;
        uint32_t temp = tmp[c];
        ext2_readBlock(fs, temp, (uint8_t*)tmp);
        tmp[d] = diskBlock;
        ext2_writeBlock(fs, temp, (uint8_t*)tmp);
        goto done;
    }
    d = c - p * p * p;
    if (d <= 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p * p - f * p);

        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS + 2]), inode, index, NULL, 0))
            ;
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[e]), inode, index, (uint8_t*)tmp,
                                             inode->blocks[EXT2_DIRECT_BLOCKS + 2]))
            ;
        uint32_t temp = tmp[e];
        ext2_readBlock(fs, tmp[e], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[f]), inode, index, (uint8_t*)tmp, temp))
            ;
        temp = tmp[f];
        ext2_readBlock(fs, tmp[f], (uint8_t*)tmp);
        tmp[g] = diskBlock;
        ext2_writeBlock(fs, temp, (uint8_t*)tmp);
        goto done;
    }

done:
    kfree(tmp);
}

// ext2_allocateBlock(ext2_t *fs) - Allocate a block from the ext2 block bitmaps
uint32_t ext2_allocateBlock(ext2_t* fs) {
    // Create a buffer of block_size to hold the bitmap
    uint32_t* buffer = kmalloc(fs->block_size);

    // Read in the inode bitmap
    for (uint32_t i = 0; i < fs->total_groups; i++) {
        if (!fs->bgd_list[i].unallocated_blocks) continue; // No free blocks available

        // Get the block usage bitmap
        uint32_t bitmapBlock = fs->bgd_list[i].block_usage_bitmap;
        ext2_readBlock(fs, bitmapBlock, (uint8_t*)buffer);

        // Iterate through it
        for (uint32_t j = 0; j < fs->block_size / 4; j++) {
            uint32_t sub_bitmap = buffer[j];
            if (sub_bitmap == 0xFFFFFFFF) continue; // Nothing

            // Iterate through each entry & check if it's free
            for (int k = 0; k < 32; k++) {
                int free = !((sub_bitmap >> k) & 0x1);
                if (free) {
                    // Set the bitmap and return
                    uint32_t mask = (0x1 << k);
                    buffer[j] = buffer[j] | mask;
                    ext2_writeBlock(fs, bitmapBlock, (uint8_t*)buffer);

                    // Update free inodes
                    fs->bgd_list[i].unallocated_blocks--;
                    ext2_rewriteBGDs(fs);

                    kfree(buffer);
                    return i * fs->blocks_per_group + j * 32 + k;
                }
            }
        }
    }

    kfree(buffer);
    panic("ext2", "ext2_allocateBlock", "No free blocks");
    return -1;
}

int allocate_block(ext2_t* fs) {
    int blockNumber = 0;
    int blockOffset = 0;
    int group = 0;

    uint8_t* bg_buffer = kmalloc(fs->block_size);

    for (uint32_t i = 0; i < fs->total_groups; i++) {
        if (fs->bgd_list[i].unallocated_blocks > 0) {
            ext2_readBlock(fs, fs->bgd_list[i].block_usage_bitmap, (uint8_t*)bg_buffer);

            while (BLOCKBIT(blockOffset)) blockOffset++;

            blockNumber = blockOffset + fs->blocks_per_group * i;
            group = i;
            break;
        }
    }

    if (!blockNumber) { panic("ext2", "ext2_allocateBlock", "No available blocks"); }

    BLOCKBYTE(blockOffset) |= SETBIT(blockOffset);
    ext2_writeBlock(fs, fs->bgd_list[group].block_usage_bitmap, (uint8_t*)bg_buffer);

    fs->bgd_list[group].unallocated_blocks--;
    ext2_rewriteBGDs(fs);

    fs->superblock->total_unallocated_blocks--;
    ext2_writeSuperBlock(fs);

    memset(bg_buffer, 0x00, fs->block_size);
    ext2_writeBlock(fs, blockNumber, bg_buffer);

    kfree(bg_buffer);

    return blockNumber;
}

// ext2_freeBlock(ext2_t *fs, uint32_t block) - Frees a block from the ext2 block bitmaps
void ext2_freeBlock(ext2_t* fs, uint32_t block) {
    uint32_t* buffer = kmalloc(fs->block_size);

    // Calculate the group and sub bitmap the block belongs to.
    uint32_t group = block / fs->blocks_per_group;
    uint32_t subBitmap = (block - (fs->blocks_per_group * group)) / 4;

    // Calculate the index in the sub bitmap
    uint32_t index = (block - (fs->blocks_per_group * group)) % 4;

    // Read in the block usage bitmap
    uint32_t bitmapBlock = fs->bgd_list[group].block_usage_bitmap;
    ext2_readBlock(fs, bitmapBlock, (uint8_t*)buffer);

    // Mask out the inode and write it back to the bitmap
    uint32_t mask = ~(0x1 << index);
    buffer[subBitmap] = buffer[subBitmap] & mask;

    ext2_writeBlock(fs, bitmapBlock, (uint8_t*)buffer);

    // Update unalloacted blocks and rewrite BGDs
    fs->bgd_list[group].unallocated_blocks++;
    ext2_rewriteBGDs(fs);
}

/* SUPERBLOCK FUNCTIONS */

// ext2_readSuperblock(fsNode_t *device) - Reads and returns the superblock for a drive
ext2_superblock_t* ext2_readSuperblock(fsNode_t* device) {
    serialPrintf("ext2_readSuperblock: Reading superblock on drive %i...\n", device->impl);

    // Allocate memory for the superblock.
    ext2_superblock_t* superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));

    // Read in the superblock
    uint8_t* buffer = kmalloc(1024);
    int ret = device->read(device, 1024, 1024, buffer); // Superblock always starts at LBA 2 and occupies 1024 bytes.

    if (ret != IDE_OK) {
        kfree(buffer);
        return NULL;
    }

    // Copy the buffer contents to the superblock and return.
    memcpy(superblock, buffer, sizeof(ext2_superblock_t));
    return superblock;
}

// ext2_writeSuperblock(ext2_t *fs) - Rewrites the superblock according to the superblock currently on the structure
int ext2_writeSuperBlock(ext2_t* fs) {
    // We cannot assume blockSize is 1024, therefore we HAVE to manually write this.
    return fs->drive->write(fs->drive, 1024, sizeof(ext2_superblock_t), (uint8_t*)(fs->superblock));
}

/* BLOCK GROUP DESCRIPTOR FUNCTIONS */

// ext2_rewriteBGDs(ext2_t *fs) - Rewrite the block group descriptors
void ext2_rewriteBGDs(ext2_t* fs) {
    for (uint32_t i = 0; i < fs->bgd_blocks; i++) {
        ext2_writeBlock(fs, 2 + i, (uint8_t*)fs->bgd_list + fs->block_size * i);
    }
}

/* INODE FUNCTIONS */

void ext2_refreshInode(ext2_t* fs, ext2_inode_t* inodet, uint32_t inode) {
    if (!inode) {
        serialPrintf("ext2_refreshInode: Cannot read inode 0\n");
        return;
    }

    inode--;

    uint32_t group = inode / fs->inodes_per_group;
    if (group > fs->total_groups) {
        serialPrintf("ext2_refreshInode: Attempted to read an inode that does not exist\n");
        return;
    }

    uint32_t inodeTableBlock = fs->bgd_list[group].inode_table;
    inode -= group * fs->inodes_per_group;
    uint32_t blockOffset = (inode * fs->superblock->extension.inode_struct_size) / fs->block_size;
    uint32_t offsetInBlock = inode - blockOffset * (fs->block_size / fs->superblock->extension.inode_struct_size);

    uint8_t* buffer = kmalloc(fs->block_size);

    ext2_readBlock(fs, inodeTableBlock + blockOffset, buffer);

    ext2_inode_t* inoder = (ext2_inode_t*)buffer;

    // QUICK FIX: Disables reading the inode if deletion time is non-zero.
    ext2_inode_t* inoderet = kmalloc(fs->superblock->extension.inode_struct_size);
    memcpy(inoderet, (uint8_t*)((uintptr_t)inoder + offsetInBlock * fs->superblock->extension.inode_struct_size),
           fs->superblock->extension.inode_struct_size);

    if (inoderet->deletion_time != 0) {
        serialPrintf("ext2_refreshInode: Deletion time is %i, refusing to read.\n", inoderet->deletion_time);
        kfree(buffer);
        kfree(inoderet);
        return;
    }

    memcpy(inodet, inoderet, fs->superblock->extension.inode_struct_size);
    kfree(buffer);
    kfree(inoderet);
}

// ext2_readInodeMetadata(ext2_t *fs, uint32_t inode) - Given an inode number, find the inode on the disk and read it.
ext2_inode_t* ext2_readInodeMetadata(ext2_t* fs, uint32_t inode) {
    ext2_inode_t* inodet = kmalloc(fs->superblock->extension.inode_struct_size);
    ext2_refreshInode(fs, inodet, inode);
    return inodet;
}

// ext2_writeInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index) - Write an inode metadata at index
int ext2_writeInodeMetadata(ext2_t* fs, ext2_inode_t* inode, uint32_t index) {
    if (!index) {
        serialPrintf("ext2_writeInodeMetadata: Attempt to write to inode 0\n");
        return -1;
    }

    index--;

    // Calculate the group
    int group = index / fs->inodes_per_group;
    if ((uint32_t)group > fs->total_groups) {
        serialPrintf("ext2_writeInodeMetadata: Invalid group!\n");
        return -1;
    }

    // Calculate the inode's table block, the offset in the table, and the offset in that block
    uint32_t inodeTableBlock = fs->bgd_list[group].inode_table;
    index -= group * fs->inodes_per_group;
    uint32_t blockOffset = (index * fs->superblock->extension.inode_struct_size) / fs->block_size;
    uint32_t offsetInBlock = index - blockOffset * (fs->block_size / fs->superblock->extension.inode_struct_size);

    // Read the current table block, write the inode, and then return.
    ext2_inode_t* inodet = kmalloc(fs->block_size);
    ext2_readBlock(fs, inodeTableBlock + blockOffset, (uint8_t*)inodet);
    memcpy((uint8_t*)((uintptr_t)inodet + offsetInBlock * fs->superblock->extension.inode_struct_size), inode,
           fs->superblock->extension.inode_struct_size);
    ext2_writeBlock(fs, inodeTableBlock + blockOffset, (uint8_t*)inodet);
    kfree(inodet);
    return 0;
}

// ext2_readInodeFiledata(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer) - Read the actual file data referenced from the inode
// ! NOT USED - DO NOT USE !
uint32_t ext2_readInodeFiledata(ext2_t* fs, ext2_inode_t* inode, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // Calculate the end offset
    uint32_t endOffset = (inode->size >= offset + size) ? (offset + size) : (inode->size);

    // Convert the offset/size to starting/ending inode block numbers
    uint32_t startBlock = offset / fs->block_size;
    uint32_t endBlock = endOffset / fs->block_size;

    // Calculate the starting block offset
    uint32_t startOffset = offset % fs->block_size;

    // Calculate how many bytes to read for the end block
    uint32_t endSize = endOffset - endBlock * fs->block_size;

    // Now, we actually read the stuff
    uint32_t i = startBlock;
    uint32_t currentOffset;
    while (i <= endBlock) {
        uint32_t left = 0, right = fs->block_size - 1;
        char* blockBuffer = kmalloc(fs->block_size);
        ext2_readInodeBlock(fs, inode, i, (uint8_t*)blockBuffer);

        if (i == startBlock) left = startOffset;
        if (i == endBlock) right = endSize - 1;

        memcpy(buffer + currentOffset, blockBuffer + left, (right - left + 1));
        currentOffset = currentOffset + (right - left + 1);

        kfree(blockBuffer);
        i++;
    }

    return endOffset - offset;
}

// ext2_allocateInodeMetadataBlock(ext2_t *fs, uint32_t *blockPtr, ext2_inode_t *inode, uint32_t index, uint8_t *buffer, uint32_t blockOverwrite) - Helper functino to allocate a block for an inode
int ext2_allocateInodeMetadataBlock(ext2_t* fs, uint32_t* blockPtr, ext2_inode_t* inode, uint32_t index,
                                    uint8_t* buffer, uint32_t blockOverwrite) {
    if (!(*blockPtr)) {
        // Allocate a block and write it either to disk or to inode metadata.
        uint32_t blockNumber = ext2_allocateBlock(fs);

        if (!blockNumber) return 1; // Could not alloacte

        *blockPtr = blockNumber;
        if (buffer)
            ext2_writeBlock(fs, blockOverwrite, (uint8_t*)buffer);
        else
            ext2_writeInodeMetadata(fs, inode, index);

        return 0;
    }
    return 1;
}

// ext2_allocateInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) - Allocate an inode block
void ext2_allocateInodeBlock(ext2_t* fs, ext2_inode_t* inode, uint32_t index, uint32_t block) {
    uint32_t ret = ext2_allocateBlock(fs);
    ext2_setDiskBlockNumber(fs, inode, index, block, ret);
    uint32_t t = (block + 1) * (fs->block_size / 512);
    if (inode->disk_sectors < t) { inode->disk_sectors = t; }

    ext2_writeInodeMetadata(fs, inode, index);
}

// ext2_freeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) - Frees an inode block
void ext2_freeInodeBlock(ext2_t* fs, ext2_inode_t* inode, uint32_t index, uint32_t block) {
    uint32_t ret = ext2_getDiskBlockNumber(fs, inode, block);
    ext2_freeBlock(fs, ret);
    ext2_setDiskBlockNumber(fs, inode, index, ret, 0);
    ext2_writeInodeMetadata(fs, inode, index);
}

// ext2_allocateInode(ext2_t *fs) - Allocate an inode from the inode bitmap
uint32_t ext2_allocateInode(ext2_t* fs) {
    uint32_t nodeNumber = 0;
    uint32_t nodeOffset = 0;
    uint32_t group = 0;
    uint8_t* bg_buffer = kmalloc(fs->block_size * 2);

    // Loop through the BGDs and try to find a free inode
    for (uint32_t i = 0; i < fs->total_groups; i++) {
        if (fs->bgd_list[i].unallocated_inodes > 0) {
            serialPrintf("ext2_allocateInode: Group %i has %i free inodes\n", i, fs->bgd_list[i].unallocated_inodes);
            ext2_readBlock(fs, fs->bgd_list[i].inode_usage_bitmap, (uint8_t*)bg_buffer);

            while (true) {

                // Iterate through the blocks
                while (BLOCKBIT(nodeOffset)) {
                    nodeOffset++;
                    if (nodeOffset == fs->inodes_per_group) goto next_block;
                }

                nodeNumber = nodeOffset + i * fs->inodes_per_group + 1;

                // Check if the inode is reserved
                if (nodeNumber <= 10) {
                    nodeOffset++;
                    if (nodeOffset == fs->inodes_per_group) goto next_block;

                    continue;
                }

                break;
            }

            if (nodeOffset == fs->inodes_per_group) {
            // Move on to the next block group
            next_block:
                serialPrintf("moving to next block group\n");
                nodeOffset = 0;
                continue;
            }

            group = i;
            break;
        }
    }

    // Make sure we actually have a free inode
    if (!nodeNumber) { panic("ext2", "ext2_allocateInode", "No free inodes available"); }

    // In ToaruOS, there is a bug with bg_buffer that can't handle block sizes >1024 (is that the actual bug?)
    // This is because bg_buffer does not read the whole contents of the inode_usage_bitmap
    // So, let's fix that
    memset(bg_buffer, 0xFF, fs->block_size * 2);
    if (fs->block_size == 4096) {
        // I used a 4096-byte block size and this bug appeared. Will test other block sizes too
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap + 1, (uint8_t*)bg_buffer + 4096);
    } else {
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
    }

    BLOCKBYTE(nodeOffset) |= SETBIT(nodeOffset);

    ext2_writeBlock(fs, (fs->bgd_list[group].inode_usage_bitmap), (uint8_t*)bg_buffer);
    if (fs->block_size == 4096)
        ext2_writeBlock(fs, (fs->bgd_list[group].inode_usage_bitmap + 1), (uint8_t*)bg_buffer + fs->block_size);
    kfree(bg_buffer);

    fs->bgd_list[group].unallocated_inodes--;
    ext2_rewriteBGDs(fs);

    fs->superblock->total_unallocated_inodes--;
    ext2_writeSuperBlock(fs);

    return nodeNumber;
}

// ext2_freeInode(ext2_t *fs, uint32_t inode) - Frees an inode from the inode bitmap
void ext2_freeInode(ext2_t* fs, uint32_t inode) {
    if (inode <= 10) {
        serialPrintf("ext2_freeInode: Cannot free a reserved inode.\n");
        return;
    }

    uint8_t* bg_buffer = kmalloc(fs->block_size * 2);

    uint32_t group = (inode - 1) / fs->inodes_per_group;

    // Make sure to do bg_buffer block sizes fix
    memset(bg_buffer, 0xFF, fs->block_size * 2);
    if (fs->block_size == 4096) {
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap + 1, (uint8_t*)bg_buffer + 4096);
    } else {
        ext2_readBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
    }

    serialPrintf("deallocating at index %i\n", inode);
    if (BLOCKBIT(inode - 1)) {
        serialPrintf("inode %i is allocated, deallocating..\n", inode);
        BLOCKBYTE(inode - 1) &= CLEARBIT(inode - 1);
    } else {
        panic("ext2", "ext2_freeInode", "Cannot free inode if it is already free");
    }

    ext2_writeBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
    if (fs->block_size == 4096)
        ext2_writeBlock(fs, (fs->bgd_list[group].inode_usage_bitmap + 1), (uint8_t*)bg_buffer + fs->block_size);

    fs->bgd_list[group].unallocated_inodes++;
    ext2_rewriteBGDs(fs);

    fs->superblock->total_unallocated_inodes++;
    ext2_writeSuperBlock(fs);
}

// (static) ext2_writeInodeBuffer(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeNumber, off_t offset, uint32_t size, uint8_t *buffer) - Write file data to an inode
static int ext2_writeInodeBuffer(ext2_t* fs, ext2_inode_t* inode, uint32_t inodeNumber, off_t offset, uint32_t size,
                                 uint8_t* buffer) {
    // Check out ext2_read() for some more information on how this actually works

    uint32_t end = offset + size;
    if (end > inode->size) {
        // We are writing more to the inode's data, need to update its size.
        inode->size = end;
        ext2_writeInodeMetadata(fs, inode, inodeNumber);
    }

    // Calculating the starting and ending block, as well as the end size and size to read.
    uint32_t startBlock = offset / fs->block_size;
    uint32_t endBlock = end / fs->block_size;
    uint32_t endSize = end - endBlock * fs->block_size;
    uint32_t sizeToRead = end - offset;

    // Now, it's writing time!
    uint8_t* buf = kmalloc(fs->block_size);
    if (startBlock == endBlock) {
        // Easy - we can just read in the current contents, sprinkle in our own, and then write it back.
        ext2_readInodeBlock(fs, inode, startBlock, buf);
        memcpy((uint8_t*)(((uintptr_t)buf) + ((uintptr_t)offset % fs->block_size)), buffer, sizeToRead);
        ext2_writeInodeBlock(fs, inode, inodeNumber, startBlock, buf);
    } else {
        uint32_t blockOffset;
        uint32_t blocksRead = 0;
        for (blockOffset = startBlock; blockOffset < endBlock; blockOffset++, blocksRead++) {
            if (blockOffset == startBlock) {
                int b = ext2_readInodeBlock(fs, inode, blockOffset, buf);
                memcpy((uint8_t*)(((uintptr_t)buf) + ((uintptr_t)offset % fs->block_size)), buffer,
                       fs->block_size - (offset % fs->block_size));
                ext2_writeInodeBlock(fs, inode, inodeNumber, blockOffset, buf);

                // Refresh the inode if we failed to read the block
                if (!b) ext2_refreshInode(fs, inode, inodeNumber);
            } else {
                int b = ext2_readInodeBlock(fs, inode, blockOffset, buf);
                memcpy(buf, buffer + fs->block_size * blocksRead - (offset % fs->block_size), fs->block_size);
                ext2_writeInodeBlock(fs, inode, inodeNumber, blockOffset, buf);

                // Refresh the inode if we failed to read the block
                if (!b) ext2_refreshInode(fs, inode, inodeNumber);
            }
        }

        if (endSize) {
            ext2_readInodeBlock(fs, inode, endBlock, buf);
            memcpy(buf, buffer + fs->block_size * blocksRead - (offset % fs->block_size), endSize);
            ext2_writeInodeBlock(fs, inode, inodeNumber, endBlock, buf);
        }
    }

    kfree(buf);
    return sizeToRead;
}

// (static) ext2_createEntry(fsNode_t *parent, char *name, uint32_t inode) - Create a new entry
static int ext2_createEntry(fsNode_t* parent, char* name, uint32_t inode) {
    ext2_t* fs = (ext2_t*)parent->impl_struct;

    // Read in the parent inode's metadata
    ext2_inode_t* pinode = ext2_readInodeMetadata(fs, parent->inode);
    if (((pinode->permissions & EXT2_INODE_DIRECTORY) == 0) || (name == NULL)) {
        serialPrintf("ext2_createEntry: Attempted to allocate an inode in a parent that was not a directory\n");
        return -1;
    }

    serialPrintf("ext2_createEntry: Creating a directory entry for %s pointing to the inode %i\n", name, inode);

    // Check how big the directory is
    serialPrintf("ext2_createEntry: Need to append %i bytes to the directory\n", sizeof(ext2_dirent_t) + strlen(name));

    uint32_t entrySize = sizeof(ext2_dirent_t) + strlen(name);
    entrySize += (entrySize % 4) ? (4 - (entrySize % 4)) : 0;

    serialPrintf("ext2_createEntry: Info about the new directory entry:\n");
    serialPrintf("\tinode = %i\n\tentrySize = %i\n\tname length = %i\n\tfile type = %i\n\tname = %s\n", inode,
                 entrySize, strlen(name), 0, name);

    serialPrintf("ext2_createEntry: Inode size is marked as %i - block size is %i\n", pinode->size, fs->block_size);
    // Time to add the entry
    uint8_t* block = kmalloc(fs->block_size);
    uint8_t blockNumber = 0;
    uint32_t dirOffset = 0, totalOffset = 0;
    int modifyOrReplace = 0;
    ext2_dirent_t* previous_entry;

    ext2_readInodeBlock(fs, pinode, blockNumber, block);

    while (totalOffset < pinode->size) {
        if (dirOffset >= fs->block_size) {
            // Offset has passed this block, move on to the next one.;
            blockNumber++;
            dirOffset = dirOffset - fs->block_size;
            ext2_readInodeBlock(fs, pinode, blockNumber, block);
        }

        ext2_dirent_t* dent = (ext2_dirent_t*)((uint32_t)block + dirOffset);

        uint32_t s_entrySize = dent->name_length + sizeof(ext2_dirent_t);
        s_entrySize += (s_entrySize % 4) ? (4 - (s_entrySize % 4)) : 0;

        // Get the name of the dirent and null-terminate it
        char f[dent->name_length + 1];
        memcpy(f, dent->name, dent->name_length);
        f[dent->name_length] = 0;

        if (dent->entry_size != s_entrySize && totalOffset + dent->entry_size == pinode->size) {
            serialPrintf(
                "ext2_createEntry: entry size should be %i but points to end of block - need to change the pointer\n",
                s_entrySize);

            dirOffset += s_entrySize;
            totalOffset += s_entrySize;

            modifyOrReplace = 1; // 1 = modify
            previous_entry = dent;

            break;
        }

        if (dent->inode == 0) modifyOrReplace = 2; // 2 = replace

        dirOffset += dent->entry_size;
        totalOffset += dent->entry_size;
    }

    if (!modifyOrReplace) serialPrintf("ext2_createEntry: modifyOrReplace = 0?\n");

    if (modifyOrReplace == 1) {
        // We need to modify the entry because the last node in the list is a real node

        if (dirOffset + entrySize >= fs->block_size) {
            // The offset and entry size are greater than the block size, we need to get another block.
            serialPrintf("ext2_createEntry: Modifying entry on new block...\n");
            blockNumber++;
            ext2_allocateInodeBlock(fs, pinode, parent->inode, blockNumber);
            memset(block, 0, fs->block_size);
            dirOffset = 0;
            pinode->size += fs->block_size;
            ext2_writeInodeMetadata(fs, pinode, parent->inode);
        } else {
            // We should be good
            serialPrintf("ext2_createEntry: Modifying entry on regular block...\n");

            uint32_t s_entrySize = previous_entry->name_length + sizeof(ext2_dirent_t);
            s_entrySize += (s_entrySize % 4) ? (4 - (s_entrySize % 4)) : 0;
            previous_entry->entry_size = s_entrySize;
            serialPrintf("ext2_createEntry: Set previous node entry size to %i\n", s_entrySize);
        }
    } else if (modifyOrReplace == 2) {
        serialPrintf("ext2_createEntry: The last node in the list is a fake node, will replace it.\n");
    }

    ext2_dirent_t* dent = (ext2_dirent_t*)((uintptr_t)block + dirOffset);

    dent->inode = inode;
    dent->entry_size = fs->block_size - dirOffset;
    dent->name_length = strlen(name);
    dent->type = 0;
    memcpy(dent->name, name, strlen(name));

    ext2_writeInodeBlock(fs, pinode, parent->inode, blockNumber, block);

    kfree(block);
    kfree(pinode);

    return -1;
}

// ext2_direntry(ext2_t *fs, ext2_inode_t *inode, uint32_t no, uint32_t index) - Function for creating dirents
ext2_dirent_t* ext2_direntry(ext2_t* fs, ext2_inode_t* inode, uint32_t no, uint32_t index) {
    uint8_t* block = kmalloc(fs->block_size);
    uint8_t blockNumber = 0;
    ext2_readInodeBlock(fs, inode, blockNumber, block);
    uint32_t dirOffset = 0;
    uint32_t totalOffset = 0;
    uint32_t dirIndex = 0;

    while (totalOffset < inode->size && dirIndex <= index) {
        ext2_dirent_t* dent = (ext2_dirent_t*)((uintptr_t)block + dirOffset);

        if (dent->inode != 0 && dirIndex == index) {
            ext2_dirent_t* out = kmalloc(dent->entry_size);
            memcpy(out, dent, dent->entry_size);
            kfree(block);
            return out;
        }

        dirOffset += dent->entry_size;
        totalOffset += dent->entry_size;

        if (dent->inode) dirIndex++;

        if (dirOffset >= fs->block_size) {
            blockNumber++;
            dirOffset -= fs->block_size;
            int ret = ext2_readInodeBlock(fs, inode, blockNumber, block);
            if (ret == -1) { break; }
        }
    }

    kfree(block);
    return NULL;
}

int ext2_mkdir(fsNode_t* parent, char* name, uint32_t permission) {
    if (!name) return -1;

    ext2_t* fs = (ext2_t*)parent->impl_struct;

    // First of all, check if it exists
    fsNode_t* check = ext2_finddir(parent, name);
    if (check) {
        serialPrintf("ext2_mkdir: A file directory by the name of %s already exists\n", name);
        kfree(check);
        return -1;
    }

    // Allocate an inode for it
    uint32_t inodeNumber = ext2_allocateInode(fs);
    ext2_inode_t* inode = ext2_readInodeMetadata(fs, inodeNumber);

    // Empty the file
    memset(inode->blocks, 0, sizeof(inode->blocks));
    inode->disk_sectors = 0;
    inode->size = 0;

    // TBD: uid and gid

    // Misc
    inode->fragment_block_addr = 0;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->os_specific1 = 0;
    inode->generation = 0;
    inode->size_high = 0;
    inode->dir_acl = 0;

    // Permissions/mode
    inode->permissions = EXT2_INODE_DIRECTORY;
    inode->permissions |= 0xFFF & permission;

    // Zero-out OS-specific 2
    memset(inode->os_specific2, 0x00, sizeof(inode->os_specific2));

    // Write out inode changes
    ext2_writeInodeMetadata(fs, inode, inodeNumber);

    // Append the entry to the parent
    ext2_createEntry(parent, name, inodeNumber);

    inode->size = fs->block_size;
    ext2_writeInodeMetadata(fs, inode, inodeNumber);

    // Writing the dirents for . and ..
    uint8_t* tmp = kmalloc(fs->block_size);
    ext2_dirent_t* t = kmalloc(12);
    memset(t, 0, 12);

    t->inode = inodeNumber;
    t->entry_size = 12;
    t->name_length = 1;
    t->name[0] = '.';

    memcpy(&tmp[0], t, 12);

    // Reuse t
    t->inode = parent->inode;
    t->name_length = 2;
    t->name[1] = '.';
    t->entry_size = fs->block_size - 12;
    memcpy(&tmp[12], t, 12);
    kfree(t);

    // Write the inode blocks
    ext2_writeInodeBlock(fs, inode, inodeNumber, 0, tmp);

    // Free some stuff
    kfree(inode);
    kfree(tmp);

    // Update the symlink count for the parent
    ext2_inode_t* pinode = ext2_readInodeMetadata(fs, parent->inode);
    pinode->hard_links++;
    ext2_writeInodeMetadata(fs, pinode, parent->inode);
    kfree(pinode);

    // Update directory count in BGD
    uint32_t group = inodeNumber / fs->inodes_per_group;
    fs->bgd_list[group].directories++;
    ext2_rewriteBGDs(fs);

    return 0;
}

// ext2_create(fsNode_t *parent, char *name, uint32_t permission) - VFS node create function
int ext2_create(fsNode_t* parent, char* name, uint32_t permission) {
    if (!name) return -1;

    ext2_t* fs = (ext2_t*)parent->impl_struct;

    // First off, check if the file already exists
    fsNode_t* check = ext2_finddir(parent, name);
    if (check) {
        serialPrintf("ext2_create: A file by the name of %s already exists\n", name);
        kfree(check);
        return -1;
    }

    // Next, allocate an inode for it.
    uint32_t inodeNumber = ext2_allocateInode(fs);
    ext2_inode_t* inode = ext2_readInodeMetadata(fs, inodeNumber);

    // Next, setup the access and creation times.
    inode->last_access = 0;
    inode->creation_time = 0;
    inode->last_modification = 0;
    inode->deletion_time = 0;

    // Now, empty the file
    memset(inode->blocks, 0, sizeof(inode->blocks));
    inode->disk_sectors = 0;
    inode->size = 0;

    // TBD: UID and GID

    // Misc
    inode->fragment_block_addr = 0;
    inode->hard_links = 1; // About to create a link
    inode->flags = 0;
    inode->os_specific1 = 0;
    inode->generation = 0;
    inode->size_high = 0;
    inode->dir_acl = 0;

    // Setup the file mode
    inode->permissions = EXT2_INODE_FILE;
    inode->permissions |= 0xFFF & permission;

    // Write the OSD blocks to 0
    memset(inode->os_specific2, 0, sizeof(inode->os_specific2));

    // Write the inode changes
    ext2_writeInodeMetadata(fs, inode, inodeNumber);

    // Append the entry to the parent
    ext2_createEntry(parent, name, inodeNumber);

    kfree(inode);
    return 0;
}

// ext2_readdir(fsNode_t *node, uint32_t index) - VFS node readdir function
struct dirent* ext2_readdir(fsNode_t* node, uint32_t index) {
    ext2_t* fs = (ext2_t*)node->impl_struct;

    ext2_inode_t* inode = ext2_readInodeMetadata(fs, node->inode);

    ext2_dirent_t* direntry = ext2_direntry(fs, inode, node->inode, index);
    if (direntry == NULL) {
        kfree(inode);
        return NULL;
    }

    // Allocate return variable, setup its parameters, and return it
    struct dirent* dirent = kmalloc(sizeof(struct dirent));
    memcpy(&dirent->name, &direntry->name, direntry->name_length);
    dirent->name[direntry->name_length] = 0;
    dirent->ino = direntry->inode;

    // this is bad
    ext2_inode_t* inodechk = ext2_readInodeMetadata(fs, dirent->ino);
    if (inodechk->deletion_time != 0 || inodechk->permissions == 0x0) {
        kfree(inodechk);
        kfree(dirent);
        return NULL;
    }

    kfree(direntry);
    kfree(inode);

    return dirent;
}

// ext2_finddir(fsNode_t *node, char *name) - VFS node finddir function
fsNode_t* ext2_finddir(fsNode_t* node, char* name) {
    ext2_t* fs = (ext2_t*)node->impl_struct;

    ext2_inode_t* inode = ext2_readInodeMetadata(fs, node->inode);
    ext2_dirent_t* dirent = NULL;
    uint8_t blockNumber = 0;

    uint8_t* block = kmalloc(fs->block_size);
    ext2_readInodeBlock(fs, inode, blockNumber, block);

    uint32_t dirOffset = 0;
    uint32_t totalOffset = 0;

    while (totalOffset < inode->size) {
        if (dirOffset >= fs->block_size) {
            blockNumber++;
            dirOffset -= fs->block_size;

            ext2_readInodeBlock(fs, inode, blockNumber, block);
        }

        ext2_dirent_t* dent = (ext2_dirent_t*)((uintptr_t)block + dirOffset);

        if (dent->inode == 0 || strlen(name) != dent->name_length) {
            dirOffset += dent->entry_size;
            totalOffset += dent->entry_size;

            continue;
        }

        char* dname = kmalloc(sizeof(char) * (dent->name_length + 1));
        memcpy(dname, &(dent->name), dent->name_length);
        dname[dent->name_length] = '\0';

        if (!strcmp(dname, name)) {
            kfree(dname);
            dirent = kmalloc(dent->entry_size);
            memcpy(dirent, dent, dent->entry_size);

            break;
        }

        kfree(dname);

        dirOffset += dent->entry_size;
        totalOffset += dent->entry_size;
    }

    kfree(block);

    if (!dirent) { return NULL; }

    inode = ext2_readInodeMetadata(fs, dirent->inode);

    // Make sure the inode wasn't deleted
    if (inode->deletion_time != 0 || inode->permissions == 0x0) {
        kfree(dirent);
        kfree(inode);
        return NULL;
    }

    // Get the file's node
    fsNode_t* ret = kmalloc(sizeof(fsNode_t));
    ext2_fileToNode(fs, dirent, inode, ret);

    kfree(dirent);
    kfree(inode);
    return ret;
}

// ext2_read(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) - VFS node read
int ext2_read(fsNode_t* node, off_t offset, uint32_t size, uint8_t* buffer) {
    ext2_t* fs = (ext2_t*)node->impl_struct;

    // Read in the inode's metadata
    ext2_inode_t* inode = ext2_readInodeMetadata(fs, node->inode);
    if (inode->size == 0) return 0;

    // We have to calculate when to stop reading, simple.
    uint32_t end;
    if (offset + size > inode->size) {
        serialPrintf("ext2_read: WARNING: Truncating size from %i to %i\n", offset + size, inode->size);
        end = inode->size; // Offset + size is too big
    } else {
        end = offset + size;
   }

    // Calculate the blocks we need to read, relative to end and offset.
    uint32_t startBlock = offset / fs->block_size;
    uint32_t endBlock = end / fs->block_size;

    // Calculate the ending size and the size to read
    uint32_t endSize = end - endBlock * fs->block_size;
    uint32_t sizeToRead = end - offset;

    // Now we can actually start reading
    uint8_t* buf = kmalloc(fs->block_size);
    if (startBlock == endBlock) {
        // Read in the inode's block
        ext2_readInodeBlock(fs, inode, startBlock, buf);
        memcpy(buffer, (uint8_t*)(((uintptr_t)buf) + ((uintptr_t)offset % fs->block_size)),
               sizeToRead); // Does a similar thing in the FAT driver
    } else {
        uint32_t blockOffset;
        uint32_t blocksRead = 0;

        for (blockOffset = startBlock; blockOffset < endBlock; blockOffset++, blocksRead++) {
            if (blockOffset == startBlock) {
                ext2_readInodeBlock(fs, inode, blockOffset, buf);
                memcpy(buffer, (uint8_t*)(((uintptr_t)buf) + ((uintptr_t)offset % fs->block_size)),
                       fs->block_size - (offset % fs->block_size)); // More complicated math
            } else {
                ext2_readInodeBlock(fs, inode, blockOffset, buf);
                memcpy(buffer + fs->block_size * blocksRead - (offset % fs->block_size), buf, fs->block_size);
            }
        }

        if (endSize) {
            ext2_readInodeBlock(fs, inode, endBlock, buf);
            memcpy(buffer + fs->block_size * blocksRead - (offset % fs->block_size), buf, endSize);
        }
    }

    kfree(inode);
    kfree(buf);

    return sizeToRead;
}

// ext2_write(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) - VFS node write function
int ext2_write(fsNode_t* node, off_t offset, uint32_t size, uint8_t* buffer) {
    ext2_t* fs = (ext2_t*)node->impl_struct;

    ext2_inode_t* inode = ext2_readInodeMetadata(fs, node->inode);

    int rv = ext2_writeInodeBuffer(fs, inode, node->inode, offset, size, buffer);
    kfree(inode);
    return rv;
}

// ext2_open(fsNode_t *file) - VFS node open function
int ext2_open(fsNode_t* file) { return 0; }

// ext2_close(fsNode_t *file) - VFS node close function
int ext2_close(fsNode_t* file) { return 0; }

void ext2_deleteInode(ext2_t* fs, ext2_inode_t* inode, uint32_t inodeNumber) {
    // Here's what we need to do:
    // (1) Iterate through each block given to the inode and free it (updating BGDs)
    // (2) Set dtime on the inode
    // (3) Free the inode in the bitmap
    panic("reduceOS", "ext2", "Deletion is deprecated. Notify author.");


}

// ext2_unlink(fsNode_t *file, char *name) - VFS node unlink function (sort of like delete)
int ext2_unlink(fsNode_t* file, char* name) {
    ext2_t* fs = (ext2_t*)file->impl_struct;

    ext2_inode_t* inode = ext2_readInodeMetadata(fs, file->inode);
    uint8_t* block = kmalloc(fs->block_size);
    ext2_dirent_t* direntry = NULL;
    uint8_t blockNumber = 0;
    ext2_readInodeBlock(fs, inode, blockNumber, block);
    uint32_t dirOffset = 0;
    uint32_t totalOffset = 0;

    while (totalOffset < inode->size) {
        // We're trying to find name in file (because file should be a directory)
        if (dirOffset >= fs->block_size) {
            // Spanning past block size, read in another.
            blockNumber++;
            dirOffset -= fs->block_size;
            ext2_readInodeBlock(fs, inode, blockNumber, block);
        }

        ext2_dirent_t* dent = (ext2_dirent_t*)((uintptr_t)block + dirOffset);

        if (dent->inode == 0 || strlen(name) != dent->name_length) {
            dirOffset += dent->entry_size;
            totalOffset += dent->entry_size;
            continue;
        }

        char* dname = kmalloc(sizeof(char) * (dent->name_length + 1));
        memcpy(dname, &(dent->name), dent->name_length);
        dname[dent->name_length] = 0;

        if (!strcmp(dname, name)) {
            kfree(dname);
            direntry = dent;
            break;
        }

        kfree(dname);

        dirOffset += dent->entry_size;
        totalOffset += dent->entry_size;
    }

    if (!direntry) {
        kfree(inode);
        kfree(block);
        return -1;
    }

    unsigned int new_inode = direntry->inode;
    ext2_writeInodeBlock(fs, inode, file->inode, blockNumber, block);
    kfree(inode);
    kfree(block);

    inode = ext2_readInodeMetadata(fs, new_inode);

    if (inode->hard_links == 1) {
        ext2_deleteInode(fs, inode, new_inode);
        serialPrintf("ext2_unlink: Deleted inode at %i\n", new_inode);
    }

    if (inode->hard_links > 0) {
        inode->hard_links--;
        ext2_writeInodeMetadata(fs, inode, new_inode);
    }

    return 0;
}

// ext2_getRoot(ext2_t *fs, ext2_inode_t *inode) - Returns the VFS node for the driver
fsNode_t* ext2_getRoot(ext2_t* fs, ext2_inode_t* inode) {
    fsNode_t* node = pmm_allocateBlocks(sizeof(fsNode_t));

    node->uid = node->gid = 0;

    // Allocate memory for the impl_struct and copy fs to it.
    node->impl_struct = (uint32_t*)fs;

    node->inode = EXT2_ROOT_INODE_NUMBER;
    node->mask = inode->permissions;

    if ((inode->permissions & EXT2_INODE_FILE) == EXT2_INODE_FILE) {
        // Oh no
        serialPrintf("ext2_getRoot: you messed something up - inode is regular file. panicking\n");
        serialPrintf("ext2_getRoot: useful information for debugging:\n");
        serialPrintf("\tuid = %i\n\tgid = %i\n\tsize = %i\n\tpermissions = %i\n\tlinks count = %i\n", inode->uid,
                     inode->gid, inode->size, inode->permissions, inode->hard_links);
        panic("ext2", "ext2_getRoot", "Inode is regular file - should not be possible.");
    }

    if ((inode->permissions & EXT2_INODE_DIRECTORY) == EXT2_INODE_DIRECTORY) {
        // everything is fine!
    } else {
        // Oh no again
        serialPrintf(
            "ext2_getRoot: oh man you messed something up - root is not a directory. THIS SHOULD BE IMPOSSIBLE\n");
        serialPrintf("ext2_getRoot: useful information for debugging:\n");
        serialPrintf("\tuid = %i\n\tgid = %i\n\tsize = %i\n\tpermissions = %i\n\tlinks count = %i\n", inode->uid,
                     inode->gid, inode->size, inode->permissions, inode->hard_links);
        panic("ext2", "ext2_getRoot", "Root is not a directory");
    }

    node->flags = VFS_DIRECTORY;
    node->read = NULL; // Root directory can't be read
    node->write = NULL;
    node->open = NULL;
    node->close = NULL;
    node->mkdir = (mkdir_t)ext2_mkdir;
    node->create = (create_t)ext2_create;
    node->readdir = (readdir_t)ext2_readdir;
    node->finddir = (finddir_t)ext2_finddir;
    node->unlink = (unlink_t)ext2_unlink;

    strcpy(node->name, "EXT2 driver");

    return node;
}

// ext2_fileToNode(ext2_t *fs, ext2_dirent_t *dirent, ext2_inode_t *inode, fsNode_t *ret) - Convert a dirent to a fsNode
int ext2_fileToNode(ext2_t* fs, ext2_dirent_t* dirent, ext2_inode_t* inode, fsNode_t* ret) {
    if (!ret) return -1;

    // Copy dirent information
    ret->impl_struct = (void*)fs;
    ret->inode = dirent->inode;
    memcpy(&ret->name, &dirent->name,
           dirent->name_length); // BUG: This isn't a full path and relies on CWD. Does it need to be a full path.
    ret->name[dirent->name_length] = '\0';

    // Copy inode information
    ret->uid = inode->uid;
    ret->gid = inode->gid;
    ret->length = inode->size;
    ret->mask = inode->permissions & 0xFFF;

    // File flags
    ret->flags = 0;
    if ((inode->permissions & EXT2_INODE_FILE) == EXT2_INODE_FILE) {
        ret->flags = VFS_FILE;
        ret->open = (open_t)ext2_open;
        ret->close = (close_t)ext2_close;
        ret->create = NULL;
        ret->read = (read_t)ext2_read;
        ret->write = (write_t)ext2_write;
        ret->finddir = NULL;
        ret->readdir = NULL;
        ret->mkdir = NULL;
        ret->unlink = NULL;
    } else if ((inode->permissions & EXT2_INODE_DIRECTORY) == EXT2_INODE_DIRECTORY) {
        ret->flags = VFS_DIRECTORY;
        ret->open = NULL;
        ret->close = NULL;
        ret->create = (create_t)ext2_create;
        ret->read = NULL;
        ret->write = NULL;
        ret->finddir = (finddir_t)ext2_finddir;
        ret->readdir = (readdir_t)ext2_readdir;
        ret->mkdir = (mkdir_t)ext2_mkdir;
        ret->unlink = (unlink_t)ext2_unlink;
    } else if (inode->permissions == 0x0) {
        ret->flags = -1;
    } else {
        serialPrintf("ext2_fileToNode: Attempt to use unimplemented type 0x%x\n", inode->permissions);
        return -1;
    }

    return 0;
}

// ext2_init() - Initializes the filesystem on a disk
fsNode_t* ext2_init(fsNode_t* node, int flags) {
    // Read in the superblock for the drive

    ext2_superblock_t* superblock = ext2_readSuperblock(node);
    if (superblock == NULL) return NULL;

    // Check the signature to see if it matches
    if (superblock->ext2_signature == EXT2_SIGNATURE) {
        serialPrintf("ext2_init: Found ext2 signature on drive %i\n", node->impl);

        // This drive has an ext2 filesystem on it, setup the ext2 filesystem object.
        ext2_t* ext2_filesystem = kmalloc(sizeof(ext2_t));
        ext2_filesystem->drive = node;
        ext2_filesystem->superblock = superblock;
        ext2_filesystem->block_size = (1024 << ext2_filesystem->superblock->unshifted_block_size);
        ext2_filesystem->blocks_per_group = ext2_filesystem->superblock->blockgroup_blocks;

        ext2_filesystem->total_groups = ext2_filesystem->superblock->total_blocks / ext2_filesystem->blocks_per_group;
        if (ext2_filesystem->blocks_per_group * ext2_filesystem->total_groups
            < ext2_filesystem->superblock->total_blocks)
            ext2_filesystem->total_groups++;

        ext2_filesystem->inodes_per_group = ext2_filesystem->superblock->total_inodes / ext2_filesystem->total_groups;

        // Calculate how many disk blocks the BGD takes
        ext2_filesystem->bgd_blocks = ext2_filesystem->total_groups * sizeof(ext2_bgd_t) / ext2_filesystem->block_size
                                      + 1;

        // Calculate BGD offset
        uint8_t bgd_offset = 2;
        if (ext2_filesystem->block_size > 1024) bgd_offset = 1;

        // Now, we need to read in the BGD blocks.
        ext2_filesystem->bgd_list = kmalloc(ext2_filesystem->block_size * ext2_filesystem->bgd_blocks);
        for (uint32_t j = 0; j < ext2_filesystem->bgd_blocks; j++) {
            ext2_readBlock(ext2_filesystem, bgd_offset + j,
                           (uint8_t*)((uintptr_t)(ext2_filesystem->bgd_list) + ext2_filesystem->block_size * j));
        }

        serialPrintf("ext2: %i BGDs, %i inodes, %i inodes per group\n", ext2_filesystem->total_groups,
                     ext2_filesystem->superblock->total_inodes, ext2_filesystem->inodes_per_group);
        serialPrintf("\t%i block size, %i BGD disk blocks, %i fragment size\n", ext2_filesystem->block_size,
                     ext2_filesystem->bgd_blocks, (1024 << ext2_filesystem->superblock->unshifted_fragment_size));
        serialPrintf("\tlast mount path: %s\n", ext2_filesystem->superblock->extension.lastPath);

// Debug the BGDS
#if 0
        char *bg_buffer = kmalloc(ext2_filesystem->block_size * sizeof(char));
        for (uint32_t j = 0; j < ext2_filesystem->total_groups; j++) {
            serialPrintf("Block Group Descriptor #%i at %i\n", j, bgd_offset + j * ext2_filesystem->superblock->blockgroup_blocks);
            serialPrintf("\tBlock Usage Bitmap at %i\n", ext2_filesystem->bgd_list[j].block_usage_bitmap);

            // Examine the block usage bitmap
            serialPrintf("\t\tExamining block bitmap at %i\n",  ext2_filesystem->bgd_list[j].block_usage_bitmap);
            ext2_readBlock(ext2_filesystem, ext2_filesystem->bgd_list[j].block_usage_bitmap, (uint8_t*)bg_buffer);
            uint32_t k = 0;
            while (BLOCKBIT(k)) ++k;

            serialPrintf("\t\tFirst free block in group is %i\n", k + ext2_filesystem->bgd_list[j].block_usage_bitmap - 2);


            serialPrintf("\tInode Bitmap at %i\n", ext2_filesystem->bgd_list[j].inode_usage_bitmap);
            
            // Examine the inode bitmap
            serialPrintf("\t\tExaming inode bitmap at %i\n", ext2_filesystem->bgd_list[j].inode_usage_bitmap);
            ext2_readBlock(ext2_filesystem, ext2_filesystem->bgd_list[j].inode_usage_bitmap, (uint8_t*)bg_buffer);
            k = 0;
            while (BLOCKBIT(k)) ++k;

            serialPrintf("\t\tFirst free inode in group is %i\n", k + ext2_filesystem->inodes_per_group * k + 1);
        }
        kfree(bg_buffer);
#endif

        // Now, we need to read in the root inode
        ext2_inode_t* rootInode = ext2_readInodeMetadata(ext2_filesystem, EXT2_ROOT_INODE_NUMBER);

        // Finally, get the VFS node
        fsNode_t* ext2 = ext2_getRoot(ext2_filesystem, rootInode);

        kfree(rootInode);

        serialPrintf("ext2_init: ext2 disk mounted\n");
        return ext2;
    } else {
        kfree(superblock);
    }

    return NULL;
}

// ext2_fs_mount(const char *device, const char *mount_path) - Mounts the etx2 filesystem
fsNode_t* ext2_fs_mount(const char* device, const char* mount_path) {
    char* arg = kmalloc(strlen((char*)device) + 1);
    strcpy(arg, device);

    char* argv[10];
    tokenize(arg, ",", argv);

    fsNode_t* dev = open_file(argv[0], 0);
    if (!dev) {
        serialPrintf("ext2_fs_mount: Could not open device %s\n", argv[0]);
        return NULL;
    }

    // No flags right now
    int flags = 0;

    return ext2_init(dev, flags);
}

// ext2_install(int argc, char *argv[]) - Installs the ext2 filesystem driver to automatically initialize on an ext2-type disk
int ext2_install(int argc, char* argv[]) {
    vfs_registerFilesystem("ext2", ext2_fs_mount);
    return 0;
}
