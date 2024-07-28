// ============================================================
// fat.c - FAT filesystem driver
// ============================================================
// This file was written for the reduceOS C kernel. Please credit me if you use this code.

#include "include/fat.h" // Main header file

// This driver is compatible with VFS standards but is missing writing functionality. That will be added later.
// DO NOT USE ANY INTERNAL FUNCTIONS. USE THE VFS ONES.

/* 
SUPPORTED FILESYSTEMS:
    - FAT12
    - FAT16
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
        for (i; i < length; i++) {
            output[i] = ' ';
        }
    }

    return 0;
}




// fatParseRootDirectory(fat_t *fs, char *path) - Locate a file or directory in the root directory
fsNode_t fatParseRootDirectory(fat_t *fs, char *path) {
    fsNode_t file;
    uint8_t *buffer = kmalloc(512);
    
    // Call helper function for 8.3 filename
    char filename[11];
    if (fatToDOSName(path, filename, 11) != 0) {
        file.flags = -1;
        return file;
    }

    filename[11] = 0;



    for (int sector = 0; sector < 14; sector++) {
        // Read in the sector.
        int lba = ((fs->drive->bpb->tableCount * fs->drive->bpb->tableSize16) + 1) + sector;
        fs->drive->driveobj->read(fs->drive->driveobj, lba * 512, 512, buffer);


        fat_fileEntry_t *directory = (fat_fileEntry_t*)buffer;

        for (int i = 0; i < 16; i++) {
            // Nasty hack we have here

            uint8_t attributes = directory->fileName[11]; // Save the attributes byte, as we need to null-terminate the string and compare.
            directory->fileName[11] = 0; // Null-terminate
                                         // Something's absolutely horrendously wrong in the stack that I can't copy this to an external variable.

           
            // strcmp() will return a 0 if the other string is totally blank, bad!
            // Solve with this if statement
            if (strlen(directory->fileName) <= 0) continue;

            if (!strcmp(directory->fileName, filename)) {
                // We found the file! Congrats!

                // Reset the attributes byte
                directory->fileName[11] = attributes;

                // Now, let's allocate a fat_t for the impl_struct and copy the contents of our directory.
                file.impl_struct = kmalloc(sizeof(fat_t));
                ((fat_t*)(file.impl_struct))->drive = fs->drive;
                ((fat_t*)(file.impl_struct))->fileEntry = kmalloc(sizeof(fat_fileEntry_t));
                memcpy(((fat_t*)(file.impl_struct))->fileEntry, directory, sizeof(fat_fileEntry_t));
                
                if (((fat_t*)file.impl_struct)->fileEntry->size != directory->size) {
                    // There was an error in the memcpy()
                    file.flags = -1;
                    return file;
                }

                

                strcpy(file.name, path); // Copy the file name into the result value
                
                // Check if its a directory or a file
                if (directory->fileName[11] == 0x10) file.flags = VFS_DIRECTORY;
                else file.flags = VFS_FILE;

                // Set the current cluster
                file.impl = ((fat_t*)file.impl_struct)->fileEntry->firstClusterNumberLow;
                
                // Setup a few other fields
                file.length = ((fat_t*)file.impl_struct)->fileEntry->size;
                

                kfree(buffer);
                return file;
            }

            directory++;
        }
        

        memset(buffer, 0, sizeof(buffer));
    }

    // Could not find file.
    kfree(buffer);
    file.flags = -1; // TBD: Make a VFS_INVALID flag
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
        else { serialPrintf("fatReadInternal: Unknown fatType!\n"); return -1; }

        uint32_t fatSector = fs->drive->firstFatSector + (fatOffset / 512);
        uint32_t entryOffset = fatOffset % 512;

        

        // Read the FATs
        fs->drive->driveobj->read(fs->drive->driveobj, fatSector * 512, 1024, (uint8_t*)FAT);
         

        // Read entry for the next cluster
        uint16_t nextCluster = *(uint16_t*)&FAT[entryOffset];

        
        if (fs->drive->fatType == 1) {
            // Convert the cluster (FAT12 only)
            nextCluster = (cluster & 1) ? nextCluster >> 4 : nextCluster & 0xFFF;
        }

        // Test for EOF
        if (nextCluster >= 0xFF8) {
            return EOF;
        }

        // Test for file corruption
        if (nextCluster == 0) {
            return EOF;
        }

        // Set the next cluster
        file->impl = nextCluster;
    }

    return 0;
}

// fatParseSubdirectory(fsNode_t file, const char *path) - Locate a file/folder in a subdirectory.
fsNode_t fatParseSubdirectory(fsNode_t file, const char *path) {
    fsNode_t ret;
    ret.impl = file.impl;
    ret.impl_struct = file.impl_struct;

    // First, get the DOS 8.3 filename.
    char filename[11];
    if (fatToDOSName(path, filename, 11) != 0) {
        ret.flags = -1;
        return ret;
    }
    filename[11] = 0;

    // Search the directory (note: just randomly picked a value for retries because orig. impl is seemingly bugged)
    for (int count = 0; count < 5; count++) {
        // Read in the directory
        uint8_t *buffer = kmalloc(512);
        
        int status = fatReadInternal(&ret, buffer, 512);
        

        // Setup the directory
        fat_fileEntry_t *directory = (fat_fileEntry_t*)buffer;

        // 16 entries in a directory
        for (int i = 0; i < 16; i++) {
            // Get the filename
            uint8_t attributes = directory->fileName[11]; // Save attributes
            directory->fileName[11] = 0;

            if (!strcmp(filename, directory->fileName)) {
                // We found it! Setup the return file.
                // Reset the attributes byte
                directory->fileName[11] = attributes;

                // Now, let's allocate a fat_t for the impl_struct and copy the contents of our directory.
                ret.impl_struct = kmalloc(sizeof(fat_t));
                ((fat_t*)(ret.impl_struct))->drive = ((fat_t*)(file.impl_struct))->drive;
                ((fat_t*)(ret.impl_struct))->fileEntry = kmalloc(sizeof(fat_fileEntry_t));
                memcpy(((fat_t*)(ret.impl_struct))->fileEntry, directory, sizeof(fat_fileEntry_t));
                
                if (((fat_t*)ret.impl_struct)->fileEntry->size != directory->size) {
                    // There was an error in the memcpy()
                    ret.flags = -1;
                    return ret;
                }


                strcpy(ret.name, path); // Copy the file name into the result value
                
                // Check if its a directory or a file
                if (directory->fileName[11] == 0x10) ret.flags = VFS_DIRECTORY;
                else ret.flags = VFS_FILE;

                // Set the current cluster
                ret.impl = ((fat_t*)ret.impl_struct)->fileEntry->firstClusterNumberLow;
                
                // Setup a few other fields
                ret.length = ((fat_t*)ret.impl_struct)->fileEntry->size;
                
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

    ret.flags = -1;
    return ret;

}


// fatOpenInternal(fsNode_t *driver, char *filename) - File open, can parse slashes and subdirectories
fsNode_t fatOpenInternal(fsNode_t *driver, char *filename) {
    fsNode_t file, directory; // directory is our current directory
    bool isRootDir = true;


    // First, we do some hacky logic to determine if the file is in a subdirectory.
    char *filepath = filename;
    char *slash = strchr(filepath, '/'); // Should definitely be updated to allow for future inode paths like /device/hda0

    if (!slash) {
        // The file is not in a subdirectory. Scan the root directory.
        directory = fatParseRootDirectory((fat_t*)(driver->impl_struct), filepath);

        if (directory.flags == VFS_FILE) {
            return directory; // Nailed it
        }

        // Failed to find
        file.flags = -1; // TBD: Make a VFS_INVALID flag.
        return file;
    }

    slash++; // Go to next character
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

        // Open subdirectory or file
        if (isRootDir) {
            directory = fatParseRootDirectory((fat_t*)(driver->impl_struct), path);
            isRootDir = false;
        } else {
            directory = fatParseSubdirectory(directory, path);
        }

        // Found directory or file?
        if (directory.flags == -1) {
            break;
        }

        // Found file?
        if (directory.flags == VFS_FILE) {
            return directory;
        }

        // Find the next '/'
        slash = strchr(slash + 1, '/');
        if (slash) slash++;
    }

    // Unable to find.
    serialPrintf("fatOpen: File %s not found.\n", filename);
    file.flags = -1;
    return file;
}

/* VFS FUNCTIONS */

uint32_t fatRead(struct fsNode *node, uint32_t off, uint32_t size, uint8_t *buf) {
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

uint32_t fatWrite(struct fsNode *node, uint32_t off, uint32_t size, uint8_t *buf) {
    return 0;
}

uint32_t fatOpen(struct fsNode *node) {
    // All we do is just pass it to fatOpenInternal basically
    // node->name should contain the filename that we want to open, and node->impl_struct should contain the fat_t object.
    // However, as I'm writing this we're currently prototyping - so let's check to make sure fat_t is there, panic if it's not.

    if (((fat_t*)(((fsNode_t*)node)->impl_struct))->drive->bpb->bootjmp[0] != 0xEB) {
        panic("FAT", "fatOpen", "bootjmp[0] is not 0xEB");
    }

    fsNode_t ret = fatOpenInternal((fsNode_t*)node, ((fsNode_t*)node)->name);
    memcpy(node, &ret, sizeof(fsNode_t));

    // Compare the two and make sure we're doing this correctly
    if (((fsNode_t*)node)->flags != ret.flags) {
        panic("FAT", "fatOpen", "memcpy() failed");
    }

    // Done!
    return 0;
}

uint32_t fatClose(struct fsNode *node) {
    return 0; // So much closing happening here
}

// fatInit(fsNode_t *driveNode) - Creates a FAT filesystem driver on driveNode and returns it
fsNode_t *fatInit(fsNode_t *driveNode) {
    serialPrintf("fatInit: FAT trying to initialize on driveNode...\n");

    // Allocate a buffer to read in the BPB
    uint32_t *buffer = kmalloc(512);
    int ret = driveNode->read(driveNode, 0, 512, buffer); // Read in the 1st sector

    if (ret != IDE_OK) {
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
        driver->drive->driveobj = driveNode;

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

        int rootOffset = (driver->drive->bpb->tableCount * driver->drive->bpb->tableSize16) + 1;        

        // Identify the FAT type.
        if (totalSectors < 4085) {
            driver->drive->fatType = 1; // FAT12
        } else if (totalSectors < 65525) {
            driver->drive->fatType = 2; // FAT16
        } else if (rootDirSectors == 0) {
            driver->drive->fatType = 3; // FAT32
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
        ret->uid = ret->gid = ret->inode = ret->impl = ret->mask = 0;
        ret->open = &fatOpen;
        ret->close = &fatClose;
        ret->create = NULL;
        ret->read = &fatRead;
        ret->write = &fatWrite;
        ret->finddir = NULL;
        ret->readdir = NULL;
        ret->mkdir = NULL;
        ret->impl_struct = driver;
        strcpy(ret->name, "FAT driver");
    
        return ret;
    }

    return NULL;
}
