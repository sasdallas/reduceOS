// initrd.h - header file for initrd.c (manages the initial ramdisk)

#ifndef INITRD_H
#define INITRD_H

// Includes
#include <stdint.h> // Integer declarations
#include <string.h> // String functions
#include <kernel/vfs.h> // Virtual File System definitions
#include <kernel/terminal.h> // printf()

// Typedefs

// Image header
typedef struct {
    uint32_t fileAmnt; // Number of files in the ramdisk.
} initrd_imageHeader_t;

// File header
typedef struct {
    uint8_t magic; // Magic number (used for error checking, should be 0xBF)
    int8_t name[64]; // Filename.
    uint32_t offset; // Offset in the ramdisk where the file starts.
    uint32_t length; // File length.
} initrd_fileHeader_t;

// Subdirectory structure
typedef struct {
    uint8_t magic; // Magic number (should be 0xBA)
    int8_t name;
    uint8_t fileMagic; // File magic number (see initrdInit())
} initrd_subdirectoryHeader_t;

// Function (there's just one lol):
fsNode_t *initrdInit(uint32_t location); // Loads initrd by defining pointers, setting values, etc.


#endif
