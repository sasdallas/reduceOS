// font.h - header file for the reduceOS font parser

#ifndef FONT_H
#define FONT_H

// Includes
#include <libk_reduced/stdint.h> // Integer declarations
#include <libk_reduced/limits.h> // Limits.
#include <kernel/heap.h> // Allocation functions.
#include <kernel/vesa.h> // VESA VBE drawing
#include <kernel/font_data.h> // Font data
#include <kernel/terminal.h>


/* Note: PSF stands for 'PC Screen Font', the font used by Linux for its console */
// Definitions

#define PSF2_MAGIC 0x864AB572


// Typedefs

typedef struct {
    uint32_t magic; // Magic bytes for identification.
    uint32_t version; // Should always be zero.
    uint32_t header_size; // Offset of bitmaps in file.
    uint32_t flags; // 0 without a unicode table.
    uint32_t glyphs; // Number of glyphs
    uint32_t bytesPerGlyph; // Size of each glyph.
    uint32_t height; // Height of glyph in pixels.
    uint32_t width; // Width of glyph in pixels.
} PSF2_Header;


typedef struct {
    uint8_t magic[2]; // Magic bytes for idnetiifcation.
    uint8_t fontMode; // PSF font mode
    uint8_t characterSize; // PSF character size.
} PSF1_Header;


// Definitions

// PSF 1 definitions
#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODEHASSEQ 0x04
#define PSF1_MAXMODE 0x05

#define PSF1_SEPARATOR 0xFFFF
#define PSF1_STARTSEQ 0xFFFE

// PSF 2 definitions
#define PSF2_HAS_UNICODE_TABLE 0x01 // Flag bit.

#define PSF2_MAXVERSION 0 // Maximum version

#define PSF2_SEPARATOR 0xFF
#define PSF2_STARTSEQ 0xFE


// Functions
void psfInit(); // Initializes the default PSF font for reduceOS.
int psfGetFontWidth(); // Get PSF font width
int psfGetFontHeight(); // Get PSF font eheight
void psfDrawChar(unsigned short int c, int cx, int cy, uint32_t fg, uint32_t bg); // Draw a PC screen font character.

#endif
