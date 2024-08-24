// bitmap.h - Header file for bitmap drawer/parser
#ifndef BITMAP_H
#define BITMAP_H

#include <string.h> // String functions
#include <stdint.h> // Integer types (uint32_t, ...)
#include <kernel/initrd.h> // Initial ramdisk support
#include <kernel/vesa.h> // VESA VBE support

// Typedefs
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t resreved2;
    uint32_t offbits;
} __attribute__((packed)) bitmap_fileHeader_t;

typedef struct {
    uint32_t size;
    long width;
    long height;
    uint16_t planes;
    uint16_t bitcount;
    uint32_t compression;
    uint32_t sizeImage;
    long XPelsPerMeter;
    long YPelsPerMeter;
    uint32_t clrUsed;
    uint32_t clrImportant;
} bitmap_infoHeader_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    char *imageBytes;
    char *buffer;
    uint32_t totalSize;
    uint32_t bpp;
    size_t bufferSize; 
    size_t offset;
} bitmap_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} palette_t;


// Functions
bitmap_t *bitmap_loadBitmap(fsNode_t *node); // Loads the bitmap into memory and returns the bitmap_t object.
#endif
