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




int fat_followClusterChain() {

}

// fat_parseRootDirectory(fat_drive_t *drive) - Parse the root directory of the drive, save the data, then move on to anotehr cluster.
void fat_parseRootDirectory(fat_drive_t *drive) {
    // Read the root directory
    uint8_t buffer[512];
    if (readRootDirectory(drive, (uint32_t)buffer) == -1) {
        serialPrintf("fat_parseRootDirectory: Error while reading root directory.\n");
        return;
    }

    // Max 16 entries per root directory
    int totalEntries = 0;
    for (int i = 0; i < 16; i++) {
        char *start_addr = &(buffer[i*32]);
        fat_fileEntry_t *fileEntry = start_addr;

        // Check the first byte of the entry - if it is 0, no more entries to parse.
        if (fileEntry->fileName[0] == 0x0) {
            serialPrintf("fat_parseRootDirectory: Done parsing - %i total entries.\n", totalEntries);
            break;
        } else if (fileEntry->fileName[0] == 0xE5) { // If the first byte is 0xE5, it is an unused entry.
            serialPrintf("fat_parseRootDirectory: Entry %i is an unused entry.\n", i);
            totalEntries += 1;
            continue;
        } else if (start_addr[11] == 0x0F) { // If the 11th byte of the entry == 0x0F, it is a LFN (long file name) entry.
            serialPrintf("fat_parseRootDirectory: Entry %i is an LFN entry - name: ", i);

            // Read the LFN entry
            fat_lfnEntry_t *lfnEntry = start_addr;
            char testFileName[26];
            for (int i = 0; i < 10; i++) {
                if (lfnEntry->firstChars[i] != 0xFF) testFileName[i] = lfnEntry->firstChars[i];
                else testFileName[i] = 0x00;
                serialPrintf("%c", testFileName[i]);
            }

            for (int i = 10; i < 22; i++) {
                if (lfnEntry->secondChars[i-10] != 0xFF) testFileName[i] = lfnEntry->secondChars[i-10];
                else testFileName[i] = 0x00;
                serialPrintf("%c", testFileName[i]);
            }

            for (int i = 22; i < 26; i++) {
                if (lfnEntry->thirdChars[i-22] != 0xFF) testFileName[i] = lfnEntry->thirdChars[i-22];
                else testFileName[i] = 0x00;
                serialPrintf("%c", testFileName[i]);
            }
            serialPrintf("\n");
        } else if (start_addr[11] == 0x10) { // If the 11th byte of the entry is 0x10, it is a directory.
            serialPrintf("fat_parseRootDirectory: Entry %i is a directory - directory name is %s\n", i, fileEntry->fileName);
        } else {
            serialPrintf("fat_parseRootDirectory: Entry %i is a file - filename is %s\n", i, fileEntry->fileName);

            // Calculate offset for the next cluster number in the FAT

            unsigned char FAT_Table[drive->bpb->bytesPerSector * 2];
            uint32_t first_cluster = fileEntry->firstClusterNumberLow | (fileEntry->firstClusterNumber << 8);
            uint32_t fat_offset = first_cluster + (first_cluster / 2);
            uint32_t fat_sector = drive->firstFatSector + (fat_offset / 512);
            uint32_t ent_offset = fat_offset % 512;
            ideReadSectors(drive->driveNum, 2, fat_sector, FAT_Table);

            uint16_t table_value = *(unsigned short*)&FAT_Table[ent_offset];
            table_value = (fileEntry->firstClusterNumber & 1) ? table_value >> 4 : table_value & 0xFFF;

            if (table_value >= 0xFF8) {
                serialPrintf("fat_parseRootDirectory: End of cluster chain.\n");
            } else {
                serialPrintf("fat_parseRootDirectory: table_value 0x%x\n", table_value);
                uint16_t first_cluster_sector = ((table_value - 2) * (uint16_t)drive->bpb->sectorsPerCluster) + (uint16_t)drive->firstDataSector;
                serialPrintf("fat_parseRootDirectory: first_cluster_sector 0x%x\n", first_cluster_sector);
            } 
        }
        totalEntries++;
    }
    return;
}

int readRootDirectory(fat_drive_t *drive, uint32_t buffer) {
    if (drive->fatType == 1 || drive->fatType == 2) {
        // In FAT12/FAT16, the first root directory is right after the FATs
        int firstRootDirectory = drive->firstDataSector - drive->rootDirSectors;
        
        // Read it
        serialPrintf("readRootDirectory: First root directory located at 0x%x\n", (uint32_t)firstRootDirectory);
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
            serialPrintf("fatInit: Sectors per cluster = %i\n", drive->bpb->sectorsPerCluster);


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

            // Read in the FATs in to an allocated memory address.
            uint16_t FAT_Table[drive->fatSize * 2];
            drive->FAT_Table = &FAT_Table;
            ideReadSectors(drive->driveNum, 2, drive->bpb->reservedSectorCount, FAT_Table);


            // Now parse the drive's root directory.
            fat_parseRootDirectory(drive);
        }
    }



    
}