// fat.h - include file for FAT

#include "include/libc/stdint.h" // Integer definitions (uint32_t, ...)
#include "include/libc/string.h" // String functions
#include "include/vfs.h" // Virtual File System
#include "include/ide_ata.h" // IDE ATA driver

// Typedefs


typedef struct {
    uint32_t tableSize32;
    uint16_t extendedFlags;
    uint16_t fatVersion;
    uint32_t rootCluster;
    uint16_t fatInfo;
    uint16_t backupSector; // Backup boot sector
    uint8_t reserved[12];
    uint8_t driveNum;
    uint8_t reserved2;
    uint8_t bootSignature;
    uint32_t volumeID;
    uint8_t volumeLabel[11];
    uint8_t fatTypeLabel[8];
} __attribute__((packed)) fat_extendedBPB32_t;

typedef struct {
    // FAT12 and FAT16 extended BPB
    uint8_t biosDriveNum;
    uint8_t reserved;
    uint8_t bootSignature;
    uint32_t volumeId;
    uint8_t volumeLabel[11];
    uint8_t fatTypeLabel[8];
} __attribute__((packed)) fat_extendedBPB16_t;

typedef struct {
    uint8_t bootjmp[3];
    uint8_t oemName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t tableCount;
    uint16_t rootEntryCount;
    uint16_t totalSectors16; // If this is zero, there are more than 65535 sectors (actual count is stored in totalSectors32)
    uint8_t mediaType;
    uint16_t tableSize16; // Sectors per FAT for FAT12/FAT16
    uint16_t sectorsPerTrack;
    uint16_t headSideCount;
    uint32_t hiddenSectorCount;
    uint32_t totalSectors32; // Total count of sectors (if greater than 65535)
    uint8_t extended[54]; // Will be typecast depending on FAT type    
} __attribute__((packed)) fat_BPB_t;

// This structure is referenced in the VFS impl_struct flag, and allocated a specific amount of memory.
// This is probably a very bad idea, but it does help cleanup code slightly.
typedef struct {
    int driveNum; // ideDevices[driveNum]
    int fatType; // 0 - exFAT, 1 - FAT12, 2 - FAT16, 3 - FAT32
    int totalSectors; // Calculated in fatInit
    int fatSize; // Calculated in fatInit
    int rootDirSectors; // Calculated in fatInit
    int dataSectors; // Calculated in fatInit
    int totalClusters; // Calculated in fatInit
    int firstDataSector; // Calculated in fatInit
    int firstFatSector; // Calculated in fatInit
    fat_BPB_t *bpb; // BPB in the FAT
    fat_extendedBPB16_t *extended16; // Is there a more memory-efficient way to do this?
    fat_extendedBPB32_t *extended32;
} __attribute__((packed)) fat_drive_t;

typedef struct {
    uint8_t fileName[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creationTime_tenths; // Tenths of a second
    uint16_t creationTime; // Hour = 5 bits, Minutes = 6 bits, Seconds = 5 bits - multiply seconds by 2.
    uint16_t creationDate; // Year = 7 bits, Month = 4 bits, Day = 5 bits
    uint16_t lastAccessDate; // Year = 7 bits, Month = 4 bits, Day = 5 bits.
    uint16_t firstClusterNumber; // High 16 bits of the entry first cluster number. Always 0 for FAT12 and FAT16
    uint16_t lastModificationTime; // Same format as creationTime
    uint16_t lastModificationDate; // Same format as creationDate
    uint16_t firstClusterNumberLow; // Low 16 bits of the entry's first cluster number (used to find the first cluster).
    uint32_t size; // Size of the file in bytes
} __attribute__((packed)) fat_fileEntry_t;

typedef struct {
    uint8_t entryOrder;
    uint8_t firstChars[10];
    uint8_t attribute; // Always 0x0F
    uint8_t longEntryType; // Zero for name entries.
    uint8_t checksum; // Checksum of the short file name.
    uint8_t secondChars[12];
    uint8_t reserved[2];
    uint8_t thirdChars[4];
} __attribute__((packed)) fat_lfnEntry_t;
