// filesystems.h - header file for the filesystem handler

#ifndef FILESYSTEMS_H
#define FILESYSTEMS_H

// Includes
#include "include/libc/stdint.h" // Integer definitions
#include "include/ide_ata.h" // IDE and ATA controllers
#include "include/vfs.h" // VFS handling

// Typedefs

// FAT (all FAT types)

// BIOS parameter block (BPB)
typedef struct {
    unsigned char OEMName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fats;
    uint16_t dirEntries;
    uint16_t sectNum;
    uint8_t media;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t headersPerCylinder;
    uint32_t hiddenSectors;
    uint32_t longSectors;
} FAT_BIOSPARAM;

// BIOS parameter block (extended) - FAT12 + FAT16
typedef struct {
    uint8_t biosDriveNumber;
    uint8_t reserved;
    uint8_t bootSignature;
    uint32_t volumeID;
    uint8_t volumeLabel[11];
    uint8_t fatTypeLabel[8];
} __attribute__((packed)) FAT_BIOSPARAMEXT_16;


// BIOS parameter block (extended) - FAT32
typedef struct {
    uint32_t sectorsPerFat_32;
    uint16_t flags;
    uint16_t fatVersion;
    uint32_t rootCluster;
    uint16_t fatInfo;
    uint16_t backupBS;
    uint8_t reserved0[12];
    uint8_t driveNum;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeID;
    uint8_t volumeLabel[11];
    uint8_t fatTypeLabel[8];
} __attribute__((packed)) FAT_BIOSPARAMEXT_32;



// A bootsector structure in FAT
typedef struct {
    uint8_t ignore[3]; // First 3 bytes are ASM instructions to jump over the BPB.
    FAT_BIOSPARAM bpb; // BIOS parameter block
    unsigned char extended[54]; // We will cast this to whatever type of extended it is.
} __attribute__((packed)) FAT_BOOTSECT;


// Directory entry structure.
typedef struct {
    uint8_t filename[8];
    uint8_t extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t timeCreated_ms;
    uint16_t timeCreated;
    uint16_t dateCreated;
    uint16_t dateLastAccessed;
    uint16_t firstClusterHighBytes;
    uint16_t lastModificationTime;
    uint16_t lastModificationDate;
    uint16_t firstCluster;
    uint32_t fileSize;
} __attribute__((packed)) FAT_DIRECTORY;

// FAT mount info
typedef struct {
    uint32_t numSectors;
    uint32_t fatOffset;
    uint32_t numRootEntries;
    uint32_t rootOffset;
    uint32_t rootSize;
    uint32_t fatSize;
    uint32_t fatEntrySize;
    uint16_t bytesPerSector;
} fatMountInfo;

// Functions
void fatGetBPB(int driveNum); // A little debug function.
void fatInit(int driveNum); // Initializes FAT on a certain drive
static void fatConvertToDOSFilename(char *directoryName, char *filename, uint32_t size); // Converts a filename to a DOS filename (what that means is uppercase and replace '.' with a ' ')
fsNode_t fatReadDirectory(const char *directoryName); // Searches the FAT to find a directory entry.
void fatRead(fsNode_t *file, uint32_t *buffer, uint32_t length); // Reads from a FAT file.
fsNode_t fatOpenSubdirectory(fsNode_t subdir, const char *filename); // Opens a subdirectory.
void fatClose(fsNode_t *file); // Closes a file.
fsNode_t fatOpen(const char *fileName); // Opens a file with a certain filename.

#endif