// ============================================================
// ext2.c - EXT2 filesystem driver for reduceOS
// ============================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/ext2.h" // Main header file

// External variables
extern ideDevice_t ideDevices[4];

/* BLOCK FUNCTIONS */

// ext2_readBlock(ext2_t *fs, uint32_t block, uint8_t *buf) - Read a block from the device
int ext2_readBlock(ext2_t *fs, uint32_t block, uint8_t *buf) {
    return fs->drive->read(fs->drive, fs->block_size * block, fs->block_size, buf);
}

// ext2_writeBlock(ext2_t *fs, uint32_t block, uint8_t *buf) - Write a block to the device
int ext2_writeBlock(ext2_t *fs, uint32_t block, uint8_t *buf) {
    return fs->drive->write(fs->drive, fs->block_size * block, fs->block_size, buf);
}

// ext2_readInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock) - Reads a block in the specified inode
uint8_t *ext2_readInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock) {
    uint8_t *buffer = kmalloc(fs->block_size);

    // Get the disk block number and read it in
    uint32_t diskBlock = ext2_getDiskBlockNumber(fs, inode, inodeBlock);
    ext2_readBlock(fs, diskBlock, buffer);

    return buffer;
}

// ext2_writeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock, uint8_t *buffer) - Writes a block in the specified inode
void ext2_writeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock, uint8_t *buffer) {
    // Get the disk block number and write it
    uint32_t diskBlock = ext2_getDiskBlockNumber(fs, inode, inodeBlock);
    ext2_writeBlock(fs, diskBlock, buffer);
}

// ext2_getDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock) - Gets the actual index of inodeBlock on the disk
uint32_t ext2_getDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t inodeBlock) {
    // This function is stupidly complex, and you know what they say: "steal from the best, invent the rest."
    // This function and ext2_setDiskBlockNumber, as well as a few others are sourced from szhou42's osdev project with a decent amount of modifications and bugfixes

    // Could be bugged because it's reading into the same buffer. Will test and find out.
    uint32_t p = fs->block_size / 4, ret;
    int b, c, d, e, f, g;
    uint32_t *tmp = kmalloc(fs->block_size);

    // Check how many blocks are left, minus direct blocks.
    int remainingBlocks = inodeBlock - EXT2_DIRECT_BLOCKS;
    if (remainingBlocks < 0) {
        ret = inode->blocks [inodeBlock];
        return ret;
    }

    b = remainingBlocks - p;
    if (b < 0) {
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS], (uint8_t*)tmp);
        ret = tmp[remainingBlocks];
        goto done; // Disgusting
    }

    c = b - p * p;
    if (c < 0) {
        c = b / p;
        d = b - c * p;
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS] + 1, (uint8_t*)tmp);
        ext2_readBlock(fs, tmp[c], (uint8_t*)tmp);
        ret = tmp[d];
        goto done;
    }

    d = c - p * p * p;
    if (d < 0) {
        e = c / (p * p);
        f = (c - e * p * p) / p;
        g = (c - e * p *p - f * p);
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (uint8_t*)tmp);
        ext2_readBlock(fs, tmp[e], (uint8_t*)tmp);
        ext2_readBlock(fs, tmp[f], (uint8_t*)tmp);
        ret = tmp[g];
        goto done;
    }

    done:
    kfree(tmp);
    return ret;
}

// ext2_setDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t inodeBlock, uint32_t diskBlock) - Sets a disk block number
void ext2_setDiskBlockNumber(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t inodeBlock, uint32_t diskBlock) {
    uint32_t p = fs->block_size / 4;
    int a, b, c, d, e, f, g;
    int iblock = inodeBlock;
    uint32_t *tmp = kmalloc(fs->block_size);

    a = iblock - EXT2_DIRECT_BLOCKS;
    if (a <= 0) {
        inode->blocks[inodeBlock] = diskBlock;
        goto done;
    }

    b = a - p;
    if (b <= 0) {
        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS]), inode, index, NULL, 0));
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
        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS + 1]), inode, index, NULL, 0));
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 1], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[c]), inode, index, (uint8_t*)tmp, inode->blocks[EXT2_DIRECT_BLOCKS + 1]));
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

        if (!ext2_allocateInodeMetadataBlock(fs, &(inode->blocks[EXT2_DIRECT_BLOCKS + 2]), inode, index, NULL, 0));
        ext2_readBlock(fs, inode->blocks[EXT2_DIRECT_BLOCKS + 2], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[e]), inode, index, (uint8_t*)tmp, inode->blocks[EXT2_DIRECT_BLOCKS + 2]));
        uint32_t temp = tmp[e];
        ext2_readBlock(fs, tmp[e], (uint8_t*)tmp);
        if (!ext2_allocateInodeMetadataBlock(fs, &(tmp[f]), inode, index, (uint8_t*)tmp, temp));
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
uint32_t ext2_allocateBlock(ext2_t *fs) {
    // Create a buffer of block_size to hold the bitmap
    uint32_t *buffer = kmalloc(fs->block_size);

    // Read in the inode bitmap
    for (int i = 0; i < fs->total_groups; i++) {
        if (!fs->bgd_list[i].unallocated_blocks) continue; // No free blocks available

        // Get the block usage bitmap
        uint32_t bitmapBlock = fs->bgd_list[i].block_usage_bitmap;
        ext2_readBlock(fs, bitmapBlock, (uint8_t*)buffer);
        
        // Iterate through it
        for (int j = 0; j < fs->block_size / 4; j++) {
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

// ext2_freeBlock(ext2_t *fs, uint32_t block) - Frees a block from the ext2 block bitmaps
void ext2_freeBlock(ext2_t *fs, uint32_t block) {
    uint32_t *buffer = kmalloc(fs->block_size); 

    // Calculate the group and sub bitmap the block belongs to.
    uint32_t group = block / fs->blocks_per_group;
    uint32_t subBitmap = (block - (fs->blocks_per_group * group)) / 4;

    // Calculate the index in the sub bitmap
    uint32_t index = (block - (fs->blocks_per_group * group)) % 4;

    // Read in the block usage bitmap
    uint32_t bitmapBlock =  fs->bgd_list[group].block_usage_bitmap;
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
ext2_superblock_t *ext2_readSuperblock(fsNode_t *device) {
    serialPrintf("ext2_readSuperblock: Reading superblock on drive %i...\n", device->impl);

    // Allocate memory for the superblock.
    ext2_superblock_t *superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
    
    // Read in the superblock
    uint8_t buffer[1024];
    int ret = device->read(device, 1024, 1024, buffer); // Superblock always starts at LBA 2 and occupies 1024 bytes.

    // Copy the buffer contents to the superblock and return.
    memcpy(superblock, buffer, sizeof(ext2_superblock_t));
    return superblock;
}



// ext2_writeSuperblock(ext2_t *fs) - Rewrites the superblock according to the superblock currently on the structure
int ext2_writeSuperBlock(ext2_t *fs) {
    // We cannot assume blockSize is 1024, therefore we HAVE to manually write this.
    fs->drive->write(fs->drive, 1024, sizeof(ext2_superblock_t), (uint8_t*)(fs->superblock));
}



/* BLOCK GROUP DESCRIPTOR FUNCTIONS */

// ext2_rewriteBGDs(ext2_t *fs) - Rewrite the block group descriptors
void ext2_rewriteBGDs(ext2_t *fs) {
    for (int i = 0; i < fs->bgd_blocks; i++) {
        serialPrintf("ext2_rewriteBGDs: Rewriting BGD %i\n", 2 + i);
        ext2_writeBlock(fs, 2 + i, (uint8_t*)fs->bgd_list +  fs->block_size * i);
    }
}


/* INODE FUNCTIONS */

// ext2_readInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index) - Given an inode number, find the inode on the disk and read it.
int ext2_readInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index) {
    // First, we need to determine which block group the inode belongs to.
    int blockGroup = (index - 1) / fs->inodes_per_group;
    if (blockGroup > fs->bgd_blocks) return -1; // Invalid block group

    // Now, get the inode table from the BGD at blockGroup and calculate the table index.
    uint32_t inode_table = fs->bgd_list[blockGroup].inode_table;
    uint32_t tableIndex = index - blockGroup * fs->inodes_per_group;

    // Find the block that inode lives in
    uint32_t blockOffset = (tableIndex - 1) * fs->superblock->extension.inode_struct_size / fs->block_size;

    // Calculate the offset within that block
    uint32_t offsetInBlock = (tableIndex - 1) - blockOffset * (fs->block_size / fs->superblock->extension.inode_struct_size);
    
    // Read in the block
    uint8_t *buffer = kmalloc(fs->block_size);
    ext2_readBlock(fs, inode_table + blockOffset, buffer);

    // Copy the contents
    memcpy(inode, buffer + offsetInBlock * fs->superblock->extension.inode_struct_size, fs->superblock->extension.inode_struct_size);
    

    return 0;
}

// ext2_writeInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index) - Write an inode metadata at index
void ext2_writeInodeMetadata(ext2_t *fs, ext2_inode_t *inode, uint32_t index) {
    // Calculate which group the inode lives in
    uint32_t group = index / fs->inodes_per_group;

    // Calculate the inode table block and the inode's block offset
    uint32_t inodeTableBlock = fs->bgd_list[group].inode_table;
    uint32_t blockOffset = (index - 1) * fs->superblock->extension.inode_struct_size / fs->block_size;

    // Calculate the offset within the block
    uint32_t offsetInBlock = (index - 1) - blockOffset * (fs->block_size / fs->superblock->extension.inode_struct_size);

    // Read in the block
    char *blockBuffer = kmalloc(fs->block_size);
    ext2_readBlock(fs, inodeTableBlock + blockOffset, blockBuffer);

    memcpy(blockBuffer + offsetInBlock * fs->superblock->extension.inode_struct_size, inode, fs->superblock->extension.inode_struct_size);
    ext2_writeBlock(fs, inodeTableBlock + blockOffset, blockBuffer);

    kfree(blockBuffer);
}


// ext2_readInodeFiledata(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer) - Read the actual file data referenced from the inode
uint32_t ext2_readInodeFiledata(ext2_t *fs, ext2_inode_t *inode, uint32_t offset, uint32_t size, uint8_t *buffer) {
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
        char *blockBuffer = ext2_readInodeBlock(fs, inode, i);

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
int ext2_allocateInodeMetadataBlock(ext2_t *fs, uint32_t *blockPtr, ext2_inode_t *inode, uint32_t index, uint8_t *buffer, uint32_t blockOverwrite) {
    if (!(*blockPtr)) {
        // Allocate a block and write it either to disk or to inode metadata.
        uint32_t blockNumber = ext2_allocateBlock(fs);

        if (!blockNumber) return 1; // Could not alloacte

        *blockPtr = blockNumber;
        if (buffer) ext2_writeBlock(fs, blockOverwrite, (uint8_t*)buffer);
        else ext2_writeInodeMetadata(fs, inode, index);

        return 0;
    }
    return 1;
}

// ext2_allocateInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) - Allocate an inode block
void ext2_allocateInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) {
    uint32_t ret = ext2_allocateBlock(fs);
    ext2_setDiskBlockNumber(fs, inode, index, block, ret);
    inode->disk_sectors = (block + 1) * (fs->block_size / 512);
    ext2_writeInodeMetadata(fs, inode, index);
}


// ext2_freeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) - Frees an inode block
void ext2_freeInodeBlock(ext2_t *fs, ext2_inode_t *inode, uint32_t index, uint32_t block) {
    uint32_t ret = ext2_getDiskBlockNumber(fs, inode, block);
    ext2_freeBlock(fs, ret);
    ext2_setDiskBlockNumber(fs, inode, index, ret, 0);
    ext2_writeInodeMetadata(fs, inode, index);
}

// ext2_allocateInode(ext2_t *fs) - Allocate an inode from the inode bitmap
uint32_t ext2_allocateInode(ext2_t *fs) {
    uint32_t nodeNumber = 0;
    uint32_t nodeOffset = 0;
    uint32_t group = 0;
    uint8_t *bg_buffer = kmalloc(fs->block_size);


    // Loop through the BGDs and try to find a free inode
    for (int i = 0; i < fs->total_groups; i++) {
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
                    if (nodeOffset = fs->inodes_per_group) goto next_block;

                    continue;
                }

                break;
            }

            if (nodeOffset == fs->inodes_per_group) {
                // Move on to the next block group
                next_block:
                    nodeOffset = 0;
                    continue;
            }

            group = i;
            break;
        }
    }

    // Make sure we actually have a free inode
    if (!nodeNumber) {
        panic("ext2", "ext2_allocateInode", "No free inodes available");
    }

    BLOCKBYTE(nodeOffset) |= SETBIT(nodeOffset);

    ext2_writeBlock(fs, fs->bgd_list[group].inode_usage_bitmap, (uint8_t*)bg_buffer);
    kfree(bg_buffer);


    fs->bgd_list[group].unallocated_inodes--;
    ext2_rewriteBGDs(fs);

    fs->superblock->total_unallocated_inodes--;
    ext2_writeSuperBlock(fs);

    return nodeNumber;
}

// ext2_freeInode(ext2_t *fs, uint32_t inode) - Frees an inode from the inode bitmap
void ext2_freeInode(ext2_t *fs, uint32_t inode) {
    uint32_t *buffer = kmalloc(fs->block_size);

    // Calculate the group, subbitmap, and the index in the sub bitmap the inode is in
    uint32_t group = inode / fs->inodes_per_group;
    uint32_t subbitmap = (inode - (fs->inodes_per_group * group)) / 4;
    uint32_t index = (inode - (fs->inodes_per_group * group)) % 4;

    // Read in the bitmap
    uint32_t bitmapBlock = fs->bgd_list[group].inode_usage_bitmap;
    ext2_readBlock(fs, bitmapBlock, (uint8_t*)buffer);

    // Mask out the inode and write it back to the bitmap
    uint32_t mask = ~(0x1 << index);
    buffer[subbitmap] = buffer[subbitmap] & mask;
    ext2_writeBlock(fs, bitmapBlock, (uint8_t*)buffer);

    // Update free inodes
    fs->bgd_list[group].unallocated_inodes++;
    ext2_rewriteBGDs(fs);
}


/* VFS FUNCTIONS */
void ext2_createEntry(fsNode_t *parent, char *name, uint32_t entryInode) {
    ext2_t *fs = (ext2_t*)parent->impl_struct;
}

void ext2_mkfile(fsNode_t *parent, char *name, uint16_t permission) {
    // Get the filesystem and inode metadata
    ext2_t *fs = (ext2_t*)(parent->impl_struct);

    uint32_t index = ext2_allocateInode(fs);
    ext2_inode_t *inode = kmalloc(sizeof(ext2_inode_t));
    ext2_readInodeMetadata(fs, inode, index);

    //serialPrintf("inode->last_access = 0x%x\n", inode->last_access);

    // Now, setup the inode
    inode->permissions = EXT2_INODE_FILE;
    inode->permissions |= 0xFFF & permission;
    inode->last_access = inode->last_modification = inode->creation_time = inode->deletion_time = inode->gid = inode->uid = 0;
    inode->fragment_block_addr = inode->disk_sectors = 0;

    inode->size = fs->block_size;
    inode->hard_links = 2;
    inode->flags = 0;
    inode->extended_attr_block = 0;
    inode->dir_acl = 0;
    inode->generation = 0;
    inode->os_specific1 = 0;

    memset(inode->blocks, 0, sizeof(inode->blocks));
    memset(inode->os_specific2, 0, 12);

    // Allocate an inode block and write the metadata
    ext2_allocateInodeBlock(fs, inode, index, 0);
    ext2_writeInodeMetadata(fs, inode, index);
    ext2_createEntry(parent, name, index);

    // Get inodep
    ext2_inode_t *inodep = kmalloc(sizeof(ext2_inode_t));
    ext2_readInodeMetadata(fs, inodep, parent->inode);
    inodep->hard_links++;
    ext2_writeInodeMetadata(fs, inodep, parent->inode);
    ext2_rewriteBGDs(fs);
}





/* OTHER FUNCTIONS */

// (static) ext2_getIDENode(int driveNum) - Creates a filesystem node for an IDE/ATA device
static fsNode_t ext2_getIDENode(int driveNum) {
    fsNode_t ret;

    ret.mask = ret.uid = ret.gid = ret.inode = ret.length = 0;
    ret.flags = VFS_DIRECTORY;
    ret.open = 0; // No lol
    ret.close = 0;
    ret.read = &ideRead_vfs;
    ret.write = &ideWrite_vfs;
    ret.readdir = 0;
    ret.finddir = 0;
    ret.impl = driveNum;
    ret.ptr = 0;

    return ret;
}


int ext2_mkdir(fsNode_t *parent, char *name, uint32_t permission) {
    if (!name) return -1;

    ext2_t *fs = parent->impl_struct;

    // First of all, check if it exists
    serialPrintf("mkdir_ext2: Skipping call to finddir_ext2(0x%x, %s)\n", parent, name);

    // Allocate an inode for it
    uint32_t inode_number = ext2_allocateInode(fs);

}

// (static) ext2_fileToNode(ext2_t *fs, ext2_dirent_t *dirent, ext2_inode_t *inode, fsNode_t *ret) - Convert a dirent to a fsNode
static int ext2_fileToNode(ext2_t *fs, ext2_dirent_t *dirent, ext2_inode_t *inode, fsNode_t *ret) {
    if (!ret)  return -1;

    // Copy dirent information
    ret->impl_struct = (void*)fs;
    ret->inode = dirent->inode;
    memcpy(&ret->name, &dirent->name, dirent->name_length);
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
        ret->open = NULL;
        ret->close = NULL;
        ret->create = NULL;
        ret->read = NULL;
        ret->write = NULL;
        ret->finddir = NULL;
        ret->readdir = NULL;
        ret->mkdir = NULL;
    } else if ((inode->permissions & EXT2_INODE_DIRECTORY) == EXT2_INODE_DIRECTORY) {
        ret->flags = VFS_DIRECTORY;
        ret->open = NULL;
        ret->close = NULL;
        ret->create = NULL;
        ret->read = NULL;
        ret->write = NULL;
        ret->finddir = NULL;
        ret->readdir = NULL;
        ret->mkdir = NULL;
    } else {
        serialPrintf("ext2_fileToNode: Attempt to use unimplemented type 0x%x\n", inode->permissions);
        return -1;
    }

    return 0;
}


fsNode_t *ext2_finddir(fsNode_t *node, char *name) {
    ext2_t *fs = (ext2_t*)node->impl_struct;

    ext2_inode_t *inode = kmalloc(sizeof(ext2_inode_t));
    ext2_readInodeMetadata(fs, inode, node->inode);

    ext2_dirent_t *dirent = NULL;
    uint8_t blockNumber = 0;

    uint8_t *block = ext2_readInodeBlock(fs, inode, blockNumber);

    uint32_t dirOffset = 0;
    uint32_t totalOffset = 0;


    while (totalOffset < inode->size) {
        if (dirOffset >= fs->block_size) {
            blockNumber++;
            dirOffset -= fs->block_size;
            kfree(block);

            block = ext2_readInodeBlock(fs, inode, blockNumber);
        }

        ext2_dirent_t *dent = (ext2_dirent_t*)((uintptr_t)block + dirOffset);
        
        if (dent->inode == 0 || strlen(name) != dent->name_length) {
            dirOffset += dent->entry_size;
            totalOffset += dent->entry_size;

            continue;
        }

        char *dname = kmalloc(sizeof(char) * (dent->name_length + 1));
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
    kfree(inode);

    if (!dirent) {
        kfree(block);
        return NULL;
    }


    memset(inode, 0, sizeof(ext2_inode_t));
    ext2_readInodeMetadata(fs, inode, dirent->inode);

    // Get the file's node
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ext2_fileToNode(fs, dirent, inode, ret);    

    kfree(dirent);
    kfree(inode);
    return ret;
}




// ext2_getRoot(ext2_t *fs, ext2_inode_t *inode) - Returns the VFS node for the driver
fsNode_t *ext2_getRoot(ext2_t *fs, ext2_inode_t *inode) {
    fsNode_t *node = pmm_allocateBlocks(sizeof(fsNode_t));

    node->uid = node->gid = 0;
    
    // Allocate memory for the impl_struct and copy fs to it.
    node->impl_struct = fs;
    
    node->inode = EXT2_ROOT_INODE_NUMBER;
    node->mask = inode->permissions;


    if ((inode->permissions & EXT2_INODE_FILE) == EXT2_INODE_FILE) {
        // Oh no
        serialPrintf("ext2_getRoot: you messed something up - inode is regular file. panicking\n");
        serialPrintf("ext2_getRoot: useful information for debugging:\n");
        serialPrintf("\tuid = %i\n\tgid = %i\n\tsize = %i\n\tpermissions = %i\n\tlinks count = %i\n", inode->uid, inode->gid, inode->size, inode->permissions, inode->hard_links);
        panic("ext2", "ext2_getRoot", "Inode is regular file - should not be possible.");
    }
    
    if ((inode->permissions & EXT2_INODE_DIRECTORY) == EXT2_INODE_DIRECTORY) {
        // everything is fine!
        serialPrintf("ext2_getRoot: inode->permissions verified\n");
    } else {
        // Oh no again
        serialPrintf("ext2_getRoot: oh man you messed something up - root is not a directory. THIS SHOULD BE IMPOSSIBLE\n");
        serialPrintf("ext2_getRoot: useful information for debugging:\n");
        serialPrintf("\tuid = %i\n\tgid = %i\n\tsize = %i\n\tpermissions = %i\n\tlinks count = %i\n", inode->uid, inode->gid, inode->size, inode->permissions, inode->hard_links);
        panic("ext2", "ext2_getRoot", "Root is not a directory");
    }

    node->flags = VFS_DIRECTORY;
    node->read = NULL;
    node->write = NULL;
    node->open = NULL;
    node->close = NULL;
    node->mkdir = NULL;
    node->create = NULL;
    node->readdir = NULL;
    node->finddir = NULL;

    
    return node;
}


// ext2_init() - Initializes the filesystem on a disk
fsNode_t *ext2_init(fsNode_t *node) {
    // Read in the superblock for the drive
    
    ext2_superblock_t *superblock = ext2_readSuperblock(node);

    // Check the signature to see if it matches
    if (superblock->ext2_signature == EXT2_SIGNATURE) {
        serialPrintf("ext2_init: Found ext2 signature on drive %i\n", node->impl);

        // This drive has an ext2 filesystem on it, setup the ext2 filesystem object.            
        ext2_t *ext2_filesystem = kmalloc(sizeof(ext2_t));
        ext2_filesystem->drive = node;
        ext2_filesystem->superblock = superblock;
        ext2_filesystem->block_size = (1024 << ext2_filesystem->superblock->unshifted_block_size);
        ext2_filesystem->blocks_per_group = ext2_filesystem->superblock->blockgroup_blocks;
        

        ext2_filesystem->total_groups = ext2_filesystem->superblock->total_blocks / ext2_filesystem->blocks_per_group;
        if (ext2_filesystem->blocks_per_group * ext2_filesystem->total_groups < ext2_filesystem->superblock->total_blocks) ext2_filesystem->total_groups++;

        ext2_filesystem->inodes_per_group = ext2_filesystem->superblock->total_inodes / ext2_filesystem->total_groups;


        // Calculate how many disk blocks the BGD takes
        ext2_filesystem->bgd_blocks = ext2_filesystem->total_groups * sizeof(ext2_bgd_t) / ext2_filesystem->block_size + 1;


        // Calculate BGD offset
        uint8_t bgd_offset = 2;
        if (ext2_filesystem->block_size > 1024) bgd_offset = 1;


        // Now, we need to read in the BGD blocks.
        ext2_filesystem->bgd_list = kmalloc(ext2_filesystem->block_size * ext2_filesystem->bgd_blocks);
        for (int j = 0; j < ext2_filesystem->bgd_blocks; j++) {
            ext2_readBlock(ext2_filesystem, bgd_offset + j, (uint8_t*)((uintptr_t)(ext2_filesystem->bgd_list) + ext2_filesystem->block_size * j));
        }

        serialPrintf("ext2: %i BGDs, %i inodes, %i inodes per group\n", ext2_filesystem->total_groups, ext2_filesystem->superblock->total_inodes, ext2_filesystem->inodes_per_group);
        serialPrintf("\t%i block size, %i BGD disk blocks, %i fragment size\n", ext2_filesystem->block_size, ext2_filesystem->bgd_blocks, (1024 << ext2_filesystem->superblock->unshifted_fragment_size));
        serialPrintf("\tlast mount path: %s\n", ext2_filesystem->superblock->extension.lastPath);

        // Debug the BGDS
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


        // Now, we need to read in the root inode
        ext2_inode_t *rootInode = kmalloc(sizeof(ext2_inode_t));
        memset(rootInode, 0, sizeof(ext2_inode_t));
        ext2_readInodeMetadata(ext2_filesystem, rootInode, EXT2_ROOT_INODE_NUMBER);

        // Finally, get the VFS node
        fsNode_t *ext2 = ext2_getRoot(ext2_filesystem, rootInode);
        

        kfree(rootInode);

        serialPrintf("ext2_init: ext2 disk mounted\n");
        return ext2;   
    } else {
        kfree(superblock);
        kfree(node);
    }

    return NULL;
}