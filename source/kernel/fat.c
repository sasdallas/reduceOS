// ============================================================
// fat.c - FAT filesystem driver
// ============================================================
// This file was written for the reduceOS C kernel. Please credit me if you use this code.

#include "include/fat.h" // Main header file

/* 
SUPPORTED FILESYSTEMS:
    - FAT12
*/

// If you want a lot of the details about FAT, see https://wiki.osdev.org/FAT

// External variables
extern ideDevice_t ideDevices[4];

// impl_struct is a reference to fat_drive_t


void parseRootDirectory(fat_drive_t *drive) {
    // Read the root directory
    uint8_t buffer[512];
    if (readRootDirectory(drive, (uint32_t)buffer) == -1) {
        serialPrintf("parseRootDirectory: Error while reading root directory.\n");
        return;
    }

    // Max 16 entries per root directory
    int totalEntries = 0;
    for (int i = 0; i < 16; i++) {
        char *start_addr = &(buffer[i*32]);
        fat_fileEntry_t *fileEntry = start_addr;

        // Check the first byte of the entry - if it is 0, no more entries to parse.
        if (fileEntry->fileName[0] == 0x0) {
            serialPrintf("parseRootDirectory: Done parsing - %i total entries.\n", totalEntries);
            break;
        }

        // If the first byte is 0xE5, it is an unused entry.
        if (fileEntry->fileName[0] == 0xE5) {
            serialPrintf("parseRootDirectory: Entry %i is an unused entry.\n", i);
            totalEntries += 1;
            continue;
        }

        // If the 11th byte of the entry == 0x0F, it is a LFN (long file name) entry.
        if (fileEntry->attributes == 0x0F) {
            serialPrintf("parseRootDirectory: Entry %i is an LFN entry\n", i);
            // Read the LFN entry
            fat_lfnEntry_t *lfnEntry = start_addr;
            char testFileName[26];
            for (int i = 0; i < 10; i++) {
                if (lfnEntry->firstChars[i] != 0xFF) testFileName[i] = lfnEntry->firstChars[i];
                else testFileName[i] = 0x00;
            }

            for (int i = 10; i < 22; i++) {
                if (lfnEntry->secondChars[i-10] != 0xFF) testFileName[i] = lfnEntry->secondChars[i-10];
                else testFileName[i] = 0x00;
            }

            for (int i = 22; i < 26; i++) {
                if (lfnEntry->thirdChars[i-22] != 0xFF) testFileName[i] = lfnEntry->thirdChars[i-22];
                else testFileName[i] = 0x00;
            }
        }

        if (fileEntry->attributes == 0x10) {
            serialPrintf("parseRootDirectory: Entry %i is a directory\n", i);
        }

        if (fileEntry->attributes == 0x0) {
            
        }
    }
}

int readRootDirectory(fat_drive_t *drive, uint32_t buffer) {
    if (drive->fatType == 1 || drive->fatType == 2) {
        // In FAT12/FAT16, the first root directory is right after the FATs
        int firstRootDirectory = drive->firstDataSector - drive->rootDirSectors;
        
        // Read it
        ideReadSectors(drive->driveNum, 1, (uint32_t)firstRootDirectory, (uint32_t)buffer);
        return 1;
    }

    return -1;
}    

void fatInit() {
    // First, find all drives that COULD have a FAT FS on them.
    serialPrintf("Searching for drive...\n");
    
    int drives[3];
    int index = 0;

    for (int i = 0; i < 4; i++) { 
        if (ideDevices[i].reserved == 1 && ideDevices[i].size > 1) {
            printf("Found IDE device with %i KB\n", ideDevices[i].size);
            drives[index] = i;
            index++;
        }
    }

    if (index == 0) {
        printf("No drives found or capacity too low to read sector.\n");
        return;
    }


    // Now, check all drives found (todo: move this to other function?)
    for (int i = 0; i < index; i++) {
        serialPrintf("fatInit: Trying drive %i...\n", drives[i]);
        // Read sector (BPB is in the first sector)
        uint32_t buffer[512];
        ideReadSectors(drives[i], 1, (uint32_t)0, buffer); // LBA = 0, SECTORS = 1
        char *start_addr = &buffer;
        
        fat_drive_t *drive = kmalloc(sizeof(fat_drive_t));
        drive->bpb = start_addr;
        serialPrintf("fatInit: Starting sequence is %x %x %x\n",drive->bpb->bootjmp[0], drive->bpb->bootjmp[1], drive->bpb->bootjmp[2]);

        // bootjmp is the ASM instruction to jump over the BPB
        // Normally, the 3 bytes are EB 3C 90, but the 3C can vary.
        if (drive->bpb->bootjmp[0] == 0xEB && drive->bpb->bootjmp[2] == 0x90) { 
            serialPrintf("fatInit: bootjmp identified\n");
            serialPrintf("fatInit: OEM is %s\n", (char*)drive->bpb->oemName);
            
            // Typecast both of the extended BPBs.
            drive->extended16 = drive->bpb->extended;
            drive->extended32 = drive->bpb->extended;

            // Create the fat_drive_t object.
            drive->driveNum = drives[i];
            

            // Now we need to identify the type of FAT used. We can do this by checking the total clusters.
            // To calculate the total clusters, we need to first calculate the total number of sectors in the volume.
            int totalSectors = (drive->bpb->totalSectors16 == 0) ? drive->bpb->totalSectors32 : drive->bpb->totalSectors16;
            drive->totalSectors = totalSectors;

            // Now, we calculate the FAT size in sectors.
            int fatSize = (drive->bpb->tableSize16 == 0) ? drive->extended32->tableSize32 : drive->bpb->tableSize16;
            drive->fatSize = fatSize;

            // Calculate the size of the root directory
            int rootDirSectors = ((drive->bpb->rootEntryCount * 32) + (drive->bpb->bytesPerSector - 1)) / drive->bpb->bytesPerSector; // If 0, this is FAT32.
            drive->rootDirSectors = rootDirSectors;

            // Calculate the total number of data sectors.
            int dataSectors = totalSectors - (drive->bpb->reservedSectorCount + (drive->bpb->tableCount * fatSize) + rootDirSectors);
            drive->dataSectors = dataSectors;

            // Finally, calculate the total number of clusters.
            int totalClusters = dataSectors / drive->bpb->sectorsPerCluster;
            drive->totalClusters = totalClusters;

            // For good measure, calculate the first data sector (where dirs and files are stored).
            int firstDataSector = drive->bpb->reservedSectorCount + (drive->bpb->tableCount * fatSize) + rootDirSectors;
            drive->firstDataSector = firstDataSector;
            
            drive->firstFatSector = drive->bpb->reservedSectorCount;

            serialPrintf("fatInit: Total sectors = %i\n", totalSectors);
            serialPrintf("fatInit: Bytes per sector = %u\n", drive->bpb->bytesPerSector);
            serialPrintf("fatInit: FAT size = %i\n", fatSize);
            serialPrintf("fatInit: Root directory sectors = %i\n", rootDirSectors);
            serialPrintf("fatInit: Data sectors = %i\n", dataSectors);
            serialPrintf("fatInit: Total clusters = %i\n", totalClusters);
            serialPrintf("fatInit: First data sector = %i\n", firstDataSector);
            serialPrintf("fatInit: First FAT sector = %i\n", drive->bpb->reservedSectorCount);

            // Identify the FAT type.
            if (totalSectors < 4085) {
                serialPrintf("fatInit: FS type is FAT12\n");
                drive->fatType = 1;
            } else if (totalSectors < 65525) {
                serialPrintf("fatInit: FS type is FAT16\n");
                drive->fatType = 2;
            } else if (rootDirSectors == 0) {
                serialPrintf("fatInit: FS type is FAT32\n");
                drive->fatType = 3;
            } else {
                serialPrintf("fatInit: FS type is most likely exFAT\n");
                drive->fatType = 0;
            }

            parseRootDirectory(drive);
        }
    }



    
}