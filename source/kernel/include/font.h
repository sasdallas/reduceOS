// font.h - header file for the reduceOS font parser

#ifndef FONT_H
#define FONT_H

// Includes
#include "include/libc/stdint.h" // Integer declarations


/* Note: PSF stands for 'PC Screen Font', the font used by Linux for its console */
// Definitions

#define PSF_MAGIC 0x864AB572


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
} PSF_Header;




#endif