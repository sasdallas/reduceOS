// ============================================================
// fat.c - FAT filesystem driver
// ============================================================
// This file was written for the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/fat.h> // Main header file

// This driver is compatible with VFS standards but is missing writing functionality. That will be added later.
// DO NOT USE ANY INTERNAL FUNCTIONS. USE THE VFS ONES.

/* 
SUPPORTED FILESYSTEMS:
    - FAT12
    - FAT16
    - FAT32
*/

// If you want a lot of the details about FAT, see https://wiki.osdev.org/FAT


// NOTE: impl_struct is a reference to fat_t

// Variables
uint8_t FAT[1024]; // File allocation table
fsNode_t driver; // Driver inode



// Functions

// fatToDOSName(char *filename, char *output, int length) - Convert a filename to DOS 8.3 file format
// NOTE: RESULT MUST BE NULL TERMINATED!!!!
int fatToDOSName(char *filename, char *output, int length) {
    if (length > 11 || strlen(filename) > length) return -1; // Users.

    memset(output, ' ', length);

    // So, the FAT filesystem stores filenames as DOS 8.3 style filenames.
    // For example, test.txt would be "TEST    TXT"
    // Another example, dir would be "DIR        "
    // NOTE: LFNs are different! They can be stored as normal filenames, like "TEST.TXT" Future support will be coming soon.

    // The first thing we need to do is convert the normal filename to uppercase, until we encounter a period.  
    bool adjustExtension = false; // Set to true if we're dealing with a file.
    int i;

    for (i = 0; i < length && i < strlen(filename); i++) {
        if (filename[i] == '.') {
            adjustExtension = true;
            output[i] = ' ';
            i++;
            break;
        } else if (filename[i] == ' ') {
            output[i] = ' ';
        } else {
            output[i] = toupper(filename[i]);
        }
    }

    if (adjustExtension) {
        for (int j = 0; j < 3; j++) {
            output[i + (8 - i) + j] = toupper(filename[i + j]);
        }
    } else if (i < length) {
        for (; i < length; i++) {
            output[i] = ' ';
        }
    }

    return 0;
}


// fatParseRootDirectory32(fat_t *fs, char *path, fsNode_t *file) - FAT32 version of fatParseRootDirectory (could probably hack in a way to use fatReadInternal, but this is easier)
int fatParseRootDirectory32(fat_t *fs, char *path, fsNode_t *file) {
    // FAT32 uses a cluster chain instead of a fixed root directory.
    
    if (file->impl) {
        // Calculate the first sector of the cluster and read it in
        int lba = ((file->impl - 2) * fs->drive->bpb->sectorsPerCluster) + fs->drive->firstDataSector;
        uint8_t *buffer = kmalloc(fs->drive->bpb->sectorsPerCluster * 512);
        fs->drive->driveobj->read(fs->drive->driveobj, lba * 512, fs->drive->bpb->sectorsPerCluster * 512, buffer);
        
        
        fat_fileEntry_t *directory = (fat_fileEntry_t*)buffer;

        for (int i = 0; i < 16; i++) {
            // Nasty hack we have here

            uint8_t attributes = directory->fileName[11]; // Save the attributes byte, as we need to null-terminate the string and compare.
            directory->fileName[11] = 0; // Null-terminate
                                         // Something's absolutely horrendously wrong in the stack that I can't copy this to an external variable.

           
            // strcmp() will return a 0 if the other string is totally blank, bad!
            // Solve with this if statement
            if (strlen((char*)(&(directory->fileName))) <= 0) continue;

            if (!strcmp((char*)&directory->fileName, path)) {
                // We found the file! Congrats!

                // Reset the attributes byte
                directory->fileName[11] = attributes;

                // Now, let's allocate a fat_t for the impl_struct and copy the contents of our directory.
                file->impl_struct = kmalloc(sizeof(fat_t));
                ((fat_t*)(file->impl_struct))->drive = fs->drive;
                ((fat_t*)(file->impl_struct))->fileEntry = kmalloc(sizeof(fat_fileEntry_t));
                memcpy(((fat_t*)(file->impl_struct))->fileEntry, directory, sizeof(fat_fileEntry_t));
                
                if (((fat_t*)file->impl_struct)->fileEntry->size != directory->size) {
                    // There was an error in the memcpy()
                    file->flags = -1;
                    return EOF;
                }

                

                strcpy(file->name, path); // Copy the file name into the result value
                
                // Check if its a directory or a file
                if (directory->fileName[11] == 0x10) file->flags = VFS_DIRECTORY;
                else file->flags = VFS_FILE;

                // Set the current cluster
                file->impl = ((fat_t*)file->impl_struct)->fileEntry->firstClusterNumberLow;
                
                // Setup a few other fields
                file->length = ((fat_t*)file->impl_struct)->fileEntry->size;
                

                kfree(buffer);
                return 1;
            }

            directory++;
        }

        // We couldn't find the file on this attempt. Let's calculate the next cluster number and try again.
        // This requires us to read in the FAT.
        
        uint32_t fatOffset = file->impl * 4;
        uint32_t fatSector = fs->drive->firstFatSector + (fatOffset / 512);
        uint32_t entryOffset = fatOffset % 512;

        // Read the FATs
        fs->drive->driveobj->read(fs->drive->driveobj, fatSector * 512, 1024, (uint8_t*)FAT);

        // Get the cluster number and mask it to ignore the high 4 bits.
        uint32_t nextCluster = *(unsigned int*)&FAT[entryOffset];
        nextCluster &= 0x0FFFFFFF;

        // Check if it's the end of the root directory, or if it is corrupt.
        if (nextCluster >= 0x0FFFFFF8 || nextCluster == 0x0FFFFFF7) return EOF;

        file->impl = nextCluster;
        return 0;
    } else {
        serialPrintf("fatParseRootDirectory32: file.impl is unavailable. Returning EOF for error.\n");
        return EOF;
    }

    return EOF; // Error.

}

// fatParseRootDirectory(fat_t *fs, char *path) - Locate a file or directory in the root directory
fsNode_t *fatParseRootDirectory(fat_t *fs, char *path) {
    fsNode_t *file = kmalloc(sizeof(fsNode_t));
    
    
    // Call helper function for 8.3 filename
    char filename[11];
    if (fatToDOSName(path, filename, 11) != 0) {
        file->flags = -1;
        return file;
    }

    filename[11] = 0;

    // FAT32 is for another function, it's more complex.
    if (fs->drive->fatType == 3) {
        // Similar to fatReadInternal, we will iterate until we've exhausted all available cluster entries.
        // Return codes: 0 = keep calling, 1 = found the file, -1 = EOF, return failure
        file->impl = fs->drive->rootOffset; // First cluster number.

        
        while (1) {
            int ret = fatParseRootDirectory32(fs, filename, file);
            if (ret == 1) return file; // Its already setup everything, we can just return the file.

            if (ret == -1) {
                file->flags = -1; // Couldn't find the file.
                return file;
            }

            if (ret != 0) {
                // There's been a problem in the code.
                serialPrintf("fatParseRootDirectory: FAT32 parsing failed!\n");
                file->flags = -1;
                return file;
            }
        }
    }


    uint8_t *buffer = kmalloc(512);

    for (int sector = 0; sector < 14; sector++) {
        // Read in the sector. This calculation differs depending on what FAT type is being used.
        // FAT12/16 have the root directory at a fixed position, and we can calculate it easily. 
        // FAT32 is where it gets tricky. FAT32 instead gives the root directory offset with a cluster, and it CAN be a cluster chain.
        // This means that extra work is put in for a FAT32 system, and is why we would call to another method to do it earlier.

        //int lba = ((fs->drive->bpb->tableCount * fs->drive->bpb->tableSize16) + 1) + sector;
        int lba = fs->drive->rootOffset + sector;
        fs->drive->driveobj->read(fs->drive->driveobj, lba * 512, 512, buffer);

        fat_fileEntry_t *directory = (fat_fileEntry_t*)buffer;

        for (int i = 0; i < 16; i++) {
            // Nasty hack we have here

            uint8_t attributes = directory->fileName[11]; // Save the attributes byte, as we need to null-terminate the string and compare.
            directory->fileName[11] = 0; // Null-terminate
                                         // Something's absolutely horrendously wrong in the stack that I can't copy this to an external variable.

           
            // strcmp() will return a 0 if the other string is totally blank, bad!
            // Solve with this if statement
            if (strlen((char*)&directory->fileName) <= 0) continue;

            if (!strcmp((char*)&directory->fileName, filename)) {
                // We found the file! Congrats!

                // Reset the attributes byte
                directory->fileName[11] = attributes;

                // Now, let's allocate a fat_t for the impl_struct and copy the contents of our directory.
                file->impl_struct = kmalloc(sizeof(fat_t));
                ((fat_t*)(file->impl_struct))->drive = fs->drive;
                ((fat_t*)(file->impl_struct))->fileEntry = kmalloc(sizeof(fat_fileEntry_t));
                memcpy(((fat_t*)(file->impl_struct))->fileEntry, directory, sizeof(fat_fileEntry_t));
                
                if (((fat_t*)file->impl_struct)->fileEntry->size != directory->size) {
                    // There was an error in the memcpy()
                    file->flags = -1;
                    return file;
                }

                

                strcpy(file->name, path); // Copy the file name into the result value
                
                // Check if its a directory or a file
                if (directory->fileName[11] == 0x10) file->flags = VFS_DIRECTORY;
                else file->flags = VFS_FILE;

                // Set the current cluster
                file->impl = ((fat_t*)file->impl_struct)->fileEntry->firstClusterNumberLow;
                
                // Setup a few other fields
                file->length = ((fat_t*)file->impl_struct)->fileEntry->size;
                

                kfree(buffer);
                return file;
            }

            directory++;
        }
        

        memset(buffer, 0, sizeof(buffer));
    }

    // Could not find file.
    kfree(buffer);
    file->flags = -1; // TBD: Make a VFS_INVALID flag
    return file;
}


// fatReadInternal(fsNode_t *file, uint8_t *buffer, uint32_t length) - Reads a file in the FAT
// NOTE: DO NOT CALL DIRECTLY. CALL fatRead AND IT WILL SOLVE ALL YOUR PROBLEMS!
int fatReadInternal(fsNode_t *file, uint8_t *buffer, uint32_t length) {
    if (file) {
        // Get the fat_t structure
        fat_t *fs = (fat_t*)(file->impl_struct);
        
        // Calculate and read in the starting physical sector
        uint16_t cluster = file->impl;
        uint32_t sector = ((cluster - 2) * fs->drive->bpb->sectorsPerCluster) + fs->drive->firstDataSector;

        uint8_t sector_buffer[fs->drive->bpb->sectorsPerCluster * 512];
        fs->drive->driveobj->read(fs->drive->driveobj, (uint32_t)sector * 512, fs->drive->bpb->sectorsPerCluster * 512, sector_buffer);
        

        
        // Copy the sector buffer to the output buffer
        uint32_t len = (length > (fs->drive->bpb->sectorsPerCluster * 512)) ? fs->drive->bpb->sectorsPerCluster * 512 : length;
        memcpy(buffer, sector_buffer, len);
 
        // Locate the File Allocation Table sector
        uint32_t fatOffset;
        if (fs->drive->fatType == 1) fatOffset = cluster + (cluster / 2);
        else if (fs->drive->fatType == 2) fatOffset = cluster * 2;
        else if (fs->drive->fatType == 3) fatOffset = cluster * 4;
        else { serialPrintf("fatReadInternal: Unknown fatType!\n"); return -1; }


        uint32_t fatSector = fs->drive->firstFatSector + (fatOffset / 512);
        uint32_t entryOffset = fatOffset % 512;

        

        // Read the FATs
        fs->drive->driveobj->read(fs->drive->driveobj, fatSector * 512, 1024, (uint8_t*)FAT);
         

        // Read entry for the next cluster
        // This should be done differently depending on the FAT version being used.
        if (fs->drive->fatType == 1 || fs->drive->fatType == 2) {
            uint16_t nextCluster = *(uint16_t*)&FAT[entryOffset];

        
            if (fs->drive->fatType == 1) {
                // Convert the cluster (FAT12 only)
                nextCluster = (cluster & 1) ? nextCluster >> 4 : nextCluster & 0xFFF;
            }

            // Test for EOF
            if ((nextCluster >= 0xFF8 && fs->drive->fatType == 1) || (nextCluster >= 0xFFF8 && fs->drive->fatType == 2)) {
                return EOF;
            }

            // Test for file corruption
            if (nextCluster == 0 || (nextCluster == 0xFFF7 && fs->drive->fatType == 2) || (nextCluster == 0xFF7 && fs->drive->fatType == 1)) {
                return EOF;
            }

            // Set the next cluster
            file->impl = nextCluster;
        } else if (fs->drive->fatType == 3) {
            // FAT32 needs to use an unsigned integer instead of a short.

            uint32_t nextCluster = *(uint32_t*)&FAT[entryOffset];
            nextCluster &= 0x0FFFFFFF;

            // Test for EOF and file corruption
            if (nextCluster >= 0x0FFFFFF8 || nextCluster == 0x0FFFFFF7) {
                return EOF;
            }

            file->impl = nextCluster;
        }
    }

    return 0;
}

// fatParseSubdirectory(fsNode_t *file, const char *path) - Locate a file/folder in a subdirectory.
fsNode_t *fatParseSubdirectory(fsNode_t *file, const char *path) {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ret->impl = file->impl;
    ret->impl_struct = file->impl_struct;

    // First, get the DOS 8.3 filename.
    char *filename = kmalloc(12);
    if (fatToDOSName((char*)path, filename, 11) != 0) {
        ret->flags = -1;
        return ret;
    }
    filename[11] = 0; 

    // Search the directory (note: just randomly picked a value for retries because orig. impl is seemingly bugged)
    for (int count = 0; count < 5; count++) {
        // Read in the directory
        uint8_t *buffer = kmalloc(512);
        
        int status = fatReadInternal(ret, buffer, 512);
        

        // Setup the directory
        fat_fileEntry_t *directory = (fat_fileEntry_t*)buffer;

        // 16 entries in a directory
        for (int i = 0; i < 16; i++) {
            // Get the filename
            uint8_t attributes = directory->fileName[11]; // Save attributes
            directory->fileName[11] = 0;


            if (!strcmp(filename, (char*)&directory->fileName)) {
                // We found it! Setup the return file.
                // Reset the attributes byte
                directory->fileName[11] = attributes;

                // Now, let's allocate a fat_t for the impl_struct and copy the contents of our directory.
                ret->impl_struct = kmalloc(sizeof(fat_t));
                ((fat_t*)(ret->impl_struct))->drive = ((fat_t*)(file->impl_struct))->drive;
                ((fat_t*)(ret->impl_struct))->fileEntry = kmalloc(sizeof(fat_fileEntry_t));
                memcpy(((fat_t*)(ret->impl_struct))->fileEntry, directory, sizeof(fat_fileEntry_t));
                
                if (((fat_t*)ret->impl_struct)->fileEntry->size != directory->size) {
                    // There was an error in the memcpy()
                    ret->flags = -1;
                    return ret;
                }


                strcpy(ret->name, path); // Copy the file name into the result value
                
                // Check if its a directory or a file
                if (directory->fileName[11] == 0x10) ret->flags = VFS_DIRECTORY;
                else ret->flags = VFS_FILE;

                // Set the current cluster
                ret->impl = ((fat_t*)ret->impl_struct)->fileEntry->firstClusterNumberLow;
                
                // Setup a few other fields
                ret->length = ((fat_t*)ret->impl_struct)->fileEntry->size;
                
                kfree(buffer);
                return ret;

            }

            directory++;
        }


        // Check if EOF
        if (status == EOF) {
            kfree(buffer);
            break;
        }
    }

    ret->flags = -1;
    return ret;

}


// fatOpenInternal(fsNode_t *driver, char *filename) - File open, can parse slashes and subdirectories
// WARNING: DO NOT USE OUTSIDE OF fatOpen - IT WILL NOT WORK!
fsNode_t *fatOpenInternal(fsNode_t *driver, char *filename) {
    fsNode_t *directory = NULL; // directory is our current directory
    bool isRootDir = true;

    // First, we do some hacky logic to determine if the file is in a subdirectory.
    char *filepath = filename;
    char *slash = strchr(filepath, '/');

    

    if (!slash) {
        // The file is not in a subdirectory. Scan the root directory.
        directory = fatParseRootDirectory((fat_t*)(driver->impl_struct), filepath);

        if (directory->flags == VFS_FILE || directory->flags == VFS_DIRECTORY) {
            return directory; // Nailed it
        }

        // Failed to find
        directory->flags = -1; // TBD: Make a VFS_INVALID flag.
        return directory;
    }

    slash++; // Go to next character

    // Okay, so there's a bit of a funny little bug in the code again.
    // It's if the user tries to pass dir/nested instead of /dir/nested.
    // The code skips to the first '/', completely ignoring the dir/
    // So we'll run a check here to make sure something's not going on here.

    if (filename[0] != '/') {
        // There's a slash, but it's not at the beginning. Reset slash to be at 0.
        slash = filepath;
    }

    while (slash) {
        // Get the path name.
        char path[16];
        int i = 0;
        for (i = 0; i < 16; i++) {
            if (slash[i] == '/' || slash[i] == '\0') break;

            // Copy the character.
            path[i] = slash[i];
        }

        path[i] = 0; // Null-terminate.

        // Check the bottom of the function for a bug description, but we'll check if the length is 0, as that means the user ended it with a "/" and nothing else
        // This means they want a directory.
        if (strlen(path) <= 0 && directory != NULL) {
            // Preventing page faults with a directory != NULL
            if (directory->flags == VFS_DIRECTORY) return directory;
        }
        
        // Open subdirectory or file
        if (isRootDir) {
            directory = fatParseRootDirectory((fat_t*)(driver->impl_struct), path);
            isRootDir = false;
        } else {
            directory = fatParseSubdirectory(directory, path);
        }

        // Found directory or file?
        if ((int)directory->flags == -1) {
            break;
        }

        // Found file?
        if (directory->flags == VFS_FILE) {
            return directory;
        }

        // Find the next '/'
        slash = strchr(slash + 1, '/');
        if (slash) slash++;
        else {
            // Here's an interesting conundrum. This function searches through subdirectories for files/directories.
            // If we want a file it's all good, it'll just return a file. But not a directory, since it might be a subdirectory that needs to be searched.
            // We can resolve this using a little else statement that says if there are no more slashes in the path and there's a directory, return it.

            // There's just a slight problem: If the user puts something like "/dir/". This will break the code, since it sees another slash and thinks "oh, we have to use that slash."
            // So we take care of that using the strlen() check further up

            if (directory->flags == VFS_DIRECTORY) return directory;
        }
    }

    // Unable to find.
    serialPrintf("fatOpen: File %s not found.\n", filename);
    directory->flags = -1;
    return directory;
}

/* VFS FUNCTIONS */

uint32_t fatRead(struct fsNode *node, off_t off, uint32_t size, uint8_t *buf) {
    // Get the fat_t structure
    fat_t *fs = (fat_t*)(((fsNode_t*)node)->impl_struct);

    // This might be inefficient, but to be honest I don't even care anymore.
    // What we're going to do is combine offset and size, read that in using our cluster, and then deal with the consequences of our actions later.
    // It will basically be like throwing memory away, but we will use kmalloc so we can return the memory (that is, assuming liballoc is initialized before FAT is)
    int total_size = off + size;

    // Now, we need to calculate the amount of clusters this size is going to span.
    // This math is weird, but that's just because we don't have a ceil() function
    // The first thing we do is calculate the amount of sectors (sectors) that total_size will span. We check if it is dividable by 512, if it is, just divide, if it isn't, round up to nearest 512 and divide.
    // Next, we calculate the amount of clusters. It's basically the same math. This can be implemented better, but it works ;)
    int sectors = (total_size % 512 == 0) ? total_size / 512 : ((total_size + 512) - (total_size % 512)) / 512;
    int clusters = (sectors % fs->drive->bpb->sectorsPerCluster == 0) ? (sectors / fs->drive->bpb->sectorsPerCluster) : ((sectors + fs->drive->bpb->sectorsPerCluster) - (sectors % fs->drive->bpb->sectorsPerCluster)) / fs->drive->bpb->sectorsPerCluster;

    // Now that we've calculated the amount of clusters, we need to read them into a temporary buffer.
    uint8_t *buffer = kmalloc(sectors * 512);


    // Iterate through each cluster and read it in
    bool earlyTermination = false; // If fatReadInternal() returns EOF, it terminated too early.
    for (int i = 0; i < clusters; i++) {
        if (fatReadInternal(node, buffer, fs->drive->bpb->sectorsPerCluster * 512) == EOF && clusters - i > 1) {
            earlyTermination = true;
            break;
        }
    }

    if (earlyTermination) {
        serialPrintf("fatRead: Too many clusters are being read! Chain terminated too early.\n"); 
        kfree(buffer);
        return -1;
    }


    // Copy based off of the offset
    memcpy(buf, buffer + off, size);


    // Free the buffer and return OK.
    kfree(buffer);
    return 0;
}

uint32_t fatWrite(struct fsNode *node, off_t off, uint32_t size, uint8_t *buf) {
    return 0;
}

int fatOpen(struct fsNode *node) {
    // All we do is just pass it to fatOpenInternal basically
    // node->name should contain the filename that we want to open, and node->impl_struct should contain the fat_t object.
    // However, as I'm writing this we're currently prototyping - so let's check to make sure fat_t is there, panic if it's not.

    // VFS is stupid so it will call this method in an attempt to open the FAT directory, which might corrupt it.
    if (!strcmp(((fsNode_t*)node)->name, "FAT driver")) { return 0; }

    if (((fat_t*)(((fsNode_t*)node)->impl_struct))->drive->bpb->bootjmp[0] != 0xEB) {
        panic("FAT", "fatOpen", "bootjmp[0] is not 0xEB");
    }




    fsNode_t *ret;
    ret = fatOpenInternal((fsNode_t*)node, ((fsNode_t*)node)->name);
    
    

    memcpy(node, ret, sizeof(fsNode_t));
    kfree(ret);

    // Compare the two and make sure we're doing this correctly
    if (((fsNode_t*)node)->flags != ret->flags) {
        panic("FAT", "fatOpen", "memcpy() failed");
    }

    // Done!
    return 0;
}

int fatClose(struct fsNode *node) {
    return 0; // So much closing happening here
}


// fatFindDirectory(struct fsNode *node, char *name) - Searches through directories to find a file
fsNode_t *fatFindDirectory(struct fsNode *node, char *name) {
    if (strlen(name) == 0) return NULL; // Users!

    char *nodename = ((fsNode_t*)node)->name;
    // There's a potential bug where if the user passes the FAT driver object, we want to change it to a "/", because FAT driver isn't a directory.
    // Plus: The VFS will try to call this driver will a full path to wherever it's mounted to, and since the mountpoint isn't actually a directory on the VFS, we get some issues.
    if (!strcmp(nodename, "FAT driver")) nodename = "/";

    // A special test case to prevent stupidity in users, where they try and run this with fatDriver as the node and "/". In an attempt to get the root directory, they pass the root directory.
    // However, this may also be because node has a name of "FAT driver" instead of "/", in which case we'll go ahead and copy the contents to a return node while changing the name.
    if (!strcmp(name, "/")) {
        // Do note that this case could also be used on a user trying to get the root of a subdirectory, which is stupid but whatever.
        fsNode_t *ret = kmalloc(sizeof(fsNode_t));
        memcpy(ret, node, sizeof(fsNode_t));
    
        if (!strcmp(ret->name, "FAT driver")) strcpy(ret->name, "/");
        return ret;
    }

    
    char *path = kmalloc(strlen(nodename) + strlen(name) + 2);
    strcpy(path, nodename);

    // Small patch to fix a potential negative bug.
    if (strlen(nodename) > 0) {
        if (nodename[strlen(nodename)-1] != '/') strcpy(path + strlen(nodename), "/"); // Adds a '/' if nodename doesn't add a slash.
    }

    strcpy(path + strlen(path), name);
    path[strlen(path)] = 0;

    // Construct the return node
    fsNode_t *ret = fatOpenInternal((fsNode_t*)node, path); // I wanted to use fatOpen, but it doesn't work. Not sure why.

    // Check the flags. finddir methods are supposed to return NULL if something doesn't exist.
    if ((int)ret->flags == -1) {
        kfree(path);
        kfree(ret);
        return NULL;
    }

    // The issue becomes that the FAT parsing methods will automatically discard the full path and only keep the file/directory name
    // Therefore, we'll have to override that.
    strcpy(ret->name, path); // This can cause bugs in the future, since I'm not sure fatOpen is prepared to deal with this.

    // Else, we can simply free the path and return the node.
    kfree(path);
    return ret;
}

// fatInit(fsNode_t *driveNode, int flags) - Creates a FAT filesystem driver on driveNode and returns it
fsNode_t *fatInit(fsNode_t *driveNode, int flags) {
    serialPrintf("fatInit: FAT trying to initialize on driveNode (drive number/impl: %i)...\n", driveNode->impl);

    // Allocate a buffer to read in the BPB
    uint32_t *buffer = kmalloc(512);
    int ret = driveNode->read(driveNode, 0, 512, (uint8_t*)buffer); // Read in the 1st sector

    if (ret != 0) {
        kfree(buffer);
        return NULL;
    }

    // Allocate memory for our structures
    fat_t *driver = kmalloc(sizeof(fat_t));
    driver->drive = kmalloc(sizeof(fat_drive_t));
    driver->drive->bpb = kmalloc(sizeof(fat_BPB_t));
    
    // Copy the BPB to the driver
    memcpy(driver->drive->bpb, buffer, 512);

    // Discard the old buffer
    kfree(buffer);

    // bootjmp is the ASM instruction to jump over the BPB
    // Normally it's EB 3C 90, but the 3C can vary
    if (driver->drive->bpb->bootjmp[0] == 0xEB && driver->drive->bpb->bootjmp[2] == 0x90) {
        serialPrintf("fatInit: bootjmp identified on drive\n");
        
        // Typecast the bpbs
        driver->drive->extended16 = (fat_extendedBPB16_t*)(driver->drive->bpb->extended);
        driver->drive->extended32 = (fat_extendedBPB32_t*)(driver->drive->bpb->extended);

        // Set the drive object
        driver->drive->driveobj = kmalloc(sizeof(fsNode_t));
        memcpy(driver->drive->driveobj, driveNode, sizeof(fsNode_t));

        // Now we need to identify the type of FAT used. We can do this by checking the total clusters.
        // To calculate the total clusters, we need to first calculate the total number of sectors in the volume.
        int totalSectors = (driver->drive->bpb->totalSectors16 == 0) ? driver->drive->bpb->totalSectors32 : driver->drive->bpb->totalSectors16;
        driver->drive->totalSectors = totalSectors;

        // Now, we calculate the FAT size in sectors.
        int fatSize = (driver->drive->bpb->tableSize16 == 0) ? driver->drive->extended32->tableSize32 : driver->drive->bpb->tableSize16;
        driver->drive->fatSize = fatSize;

        // Calculate the size of the root directory
        int rootDirSectors = ((driver->drive->bpb->rootEntryCount * 32) + (driver->drive->bpb->bytesPerSector - 1)) / driver->drive->bpb->bytesPerSector; // If 0, this is FAT32.
        driver->drive->rootDirSectors = rootDirSectors;

        // Calculate the total number of data sectors.
        int dataSectors = totalSectors - (driver->drive->bpb->reservedSectorCount + (driver->drive->bpb->tableCount * fatSize) + rootDirSectors);
        driver->drive->dataSectors = dataSectors;

        // Finally, calculate the total number of clusters.
        int totalClusters = dataSectors / driver->drive->bpb->sectorsPerCluster;
        driver->drive->totalClusters = totalClusters;

        // For good measure, calculate the first data sector (where dirs and files are stored).
        int firstDataSector = driver->drive->bpb->reservedSectorCount + (driver->drive->bpb->tableCount * fatSize) + rootDirSectors;
        driver->drive->firstDataSector = firstDataSector;
            
        driver->drive->firstFatSector = driver->drive->bpb->reservedSectorCount;

        // The rootOffset may be calculated differently depending on the FAT filesystem.
        int rootOffset = (driver->drive->bpb->tableCount * driver->drive->bpb->tableSize16) + 1;        
        driver->drive->rootOffset = rootOffset;

        // Identify the FAT type.
        if (totalSectors < 4085) {
            driver->drive->fatType = 1; // FAT12
        } else if (totalSectors < 65525) {
            driver->drive->fatType = 2; // FAT16
        } else if (rootDirSectors == 0) {
            driver->drive->fatType = 3; // FAT32
            serialPrintf("fatInit: Detected a FAT32 filesystem. Reading in and verifying FSInfo structure...\n");
            
            // FSInfo is a structure that is required for FAT32 drivers. The sector number can be found in the extended BPB for FAT32.
            fat_fsInfo_t *fsInfo = kmalloc(512);
            int ret = driveNode->read(driveNode, 512 * driver->drive->extended32->fatInfo, 512, (uint8_t*)fsInfo);
            if (ret != 0) {
                serialPrintf("fatInit: Failed to read the FSInfo structure.\n");
                kfree(fsInfo);
                kfree(driver->drive->bpb);
                kfree(driver->drive);
                kfree(driver);
                return NULL;
            }

            // Validate the fsInfo signatures
            if (fsInfo->signature != 0x41615252 || fsInfo->signature2 != 0x61417272 || fsInfo->signature3 != 0xAA550000) {
                serialPrintf("fatInit: FSInfo signatures invalid!\n\tSignature 1 = 0x%x\n\tSignature 2 = 0x%x\n\tTrailing signature = 0x%x\n", fsInfo->signature, fsInfo->signature2, fsInfo->signature3);
                kfree(driver->drive->bpb);
                kfree(driver->drive);
                kfree(driver);
                kfree(fsInfo);
                return NULL;
            }

            // The fsInfo structure mandates that the last known free cluster count be checked and recomputed if needed.
            // It also should be range checked that it is avctually valid.
            if (fsInfo->freeClusterCount == 0xFFFFFFFF || fsInfo->freeClusterCount > (uint32_t)driver->drive->totalClusters) {
                serialPrintf("fatInit: WARNING! Free cluster count needs to be recomputed. THIS IS TBD\n");
            }

            // The same should be done for the starting number for available clusters. We don't use this number for now, but it would be nice to have later.
            // This number will likely come in handy when write functions are added, as during cluster allocation it should be modified
            if (fsInfo->availableClusterStart == 0xFFFFFFFF || fsInfo->availableClusterStart > (uint32_t)driver->drive->totalClusters) {
                serialPrintf("fatInit: WARNING! Starting cluster number needs to be recomputed. Assuming 2.\n"); // TBD
            }


            driver->drive->fsInfo = fsInfo;

            // We also need to calculate the root offset
            driver->drive->rootOffset = driver->drive->extended32->rootCluster;

        } else {
            driver->drive->fatType = 0; // Unknown/unsupported. Return NULL.
            kfree(driver->drive->bpb);
            kfree(driver->drive);
            kfree(driver);
            serialPrintf("fatInit: Attempt to initialize on unknown FAT type!\n");
            return NULL;
        }



        // Create the VFS node and return it
        fsNode_t *ret = kmalloc(sizeof(fsNode_t));
        ret->flags = VFS_DIRECTORY;
        ret->uid = ret->gid = ret->inode = ret->impl = ret->mask = 0;
        ret->open = (open_t)fatOpen;
        ret->close = (close_t)fatClose;
        ret->create = NULL;
        ret->read = (read_t)fatRead;
        ret->write = (write_t)fatWrite;
        ret->finddir = (finddir_t)fatFindDirectory;
        ret->readdir = NULL;
        ret->mkdir = NULL;
        ret->impl_struct = (uint32_t*)driver;
        strcpy(ret->name, "FAT driver");
    
        return ret;
    }

    return NULL;
}

struct dirent *fat_readdir(fsNode_t *node, uint32_t index) {
    // Okay, so first thing we need to do is calculate the path.
    // Because a user might try to list the root, and since root's name is FAT driver, that's not gonna happen

    char *path = kmalloc(strlen(node->name));
    if (!strcmp(path, "FAT driver")) strcpy(path, "/");
    else strcpy(path, node->name);

    // This is not going to be easy to implement.
    // Not sure how I'm gonna do it while at least looking somewhat decent...

    panic("FAT", "fat_readdir", "Unimplemented");
    __builtin_unreachable();
}


// fat_fs_mount(const char *device, const char *mount_path) - Mounts the FAT filesystem
fsNode_t *fat_fs_mount(const char *device, const char *mount_path) {
    char *arg = kmalloc(strlen((char*)device) + 1);
    strcpy(arg, device);

    char *argv[10];
    tokenize(arg, ",", argv);

    fsNode_t *dev = open_file(argv[0], 0);
    if (!dev) {
        serialPrintf("fat_fs_mount: Could not open device %s\n", argv[0]);
        return NULL;
    }


    // No flags right now
    int flags = 0;

    fsNode_t *fat = fatInit(dev, flags);
    serialPrintf("%s mounted\n", fat->name);
    return fat;
}

// fat_install(int argc, char *argv[]) - Installs the FAT filesystem driver to automatically initialize on an ext2-type disk
int fat_install(int argc, char *argv[]) {
    vfs_registerFilesystem("fat", fat_fs_mount);
    return 0;
}
