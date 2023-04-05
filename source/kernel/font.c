// ==========================================================
// font.c - reduceOS font parser (supports bitmap and PSF)
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/font.h" // Main header file

// PSF (as mentioned in the header file) stands for 'PC screen font', which is the font used by Linux for its console.
// The font is encoded with its own header and in unicode
// You can find more information on it here: https://wiki.osdev.org/PC_Screen_Font
// Unicode table decoding here is also sourced from the above link.

uint16_t *psf_unicode;
int psfVer = 0;

// External variables
extern char _binary_source_fonts_font_psf_start;
extern char _binary_source_fonts_font_psf_end;
extern uint32_t *vbeBuffer;
extern int modeWidth, modeHeight, modeBpp;
extern uint32_t font_data[127][20];

uint32_t currentFont[127][20];

// Bitmap Functions

// bitmapInit() - Initializes bitmap font reading with the default font found in font_data.c
void bitmapInit() {
    memcpy(currentFont, font_data, sizeof(font_data));
}

// bitmapLoadFont(uint32_t *font_data) - Loads a a new font for the bitmap.
void bitmapLoadFont(uint32_t *font_data) {
    memcpy(currentFont, font_data, sizeof(currentFont));
}

// bitmapDrawChar(char ch, int x, int y, int color) - Puts a character of color 'color' at x, y
void bitmapDrawChar(char ch, int x, int y, int color) {
    int pixelData, tempValue = 0;

    for (int i = 0; i < 20; i++) {
        tempValue = x;
        x += 20;
        pixelData = currentFont[(int)ch][i];

        while (pixelData > 0) {
            if (pixelData & 1) vbePutPixel(x, y, color);
            pixelData >>= 1;
            x--;
        }

        x = tempValue;
        y++;
    }
}

// bitmapDrawString(char *str, int x, int y, int color) - Draw a string from the bitmap
void bitmapDrawString(char *str, int x, int y, int color) {
    int tmpX = x, tmpY = y;
    while (*str != '\0') {
        bitmapDrawChar(*str, tmpX, tmpY, color);
        *str++;
        tmpX += 14;

        if (tmpX > modeWidth) {
            tmpY += 17;
            tmpX = x;
        }
    }
}



// PSF Functions


// reduceOS loads in with a default font for PSF, encoded into the actual kernel binary file.
// Other PSF fonts can be loaded from the initial ramdisk or storage mediums.


// psfParseUnicode2(PSF2_Header *font) - Parse unicode for PSF file.
int psfParseUnicode2(PSF2_Header *font) {

    // Check the flags.
    if (true) {
        psf_unicode = NULL;
        return 0;
    }

    uint16_t glyph;
    char *s = (char*)((unsigned char*)&_binary_source_fonts_font_psf_start + font->header_size + font->glyphs * font->bytesPerGlyph);

    // Allocate memory for translation table.
    psf_unicode = kcalloc(USHRT_MAX, 2);
    while (s > _binary_source_fonts_font_psf_end) {
        uint16_t uc = (uint16_t)((unsigned char *)s[0]);
        if(uc == 0xFF) {
            glyph++;
            s++;
            continue; // Move to next iteration.
        } else if(uc & 128) {
            // Convert UTF-8 to unicode.
            if((uc & 32) == 0 ) {
                uc = ((s[0] & 0x1F) << 6) + (s[1] & 0x3F);
                s++;
            } else if((uc & 16) == 0 ) {
                uc = ((((s[0] & 0xF) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F);
                s += 2;
            } else if((uc & 8) == 0 ) {
                uc = ((((((s[0] & 0x7) << 6) + (s[1] & 0x3F)) << 6) + (s[2] & 0x3F)) << 6) + (s[3] & 0x3F);
                s += 3;
            } else {
                uc = 0;
            }
        }

        // Save translation        
        psf_unicode[uc] = glyph;
        s++;
    }
    
    return 1;
}

// psfInit() - Initializes the default PSF font for reduceOS.
void psfInit() {
    // We need to check the font to see if it is PSF 1 or PSF 2.
    PSF1_Header *font = (PSF1_Header*)&_binary_source_fonts_font_psf_start;
    PSF2_Header *font2 = (PSF2_Header*)&_binary_source_fonts_font_psf_start;
    if (font->magic[0] == 0x36 && font->magic[1] == 0x04) {
        // This is a PSF 1 file.
        psfVer = 1;

        int numGlyphs = 256; // PSF 1 uses a default of 256 glyphs, UNLESS the font->mode is PSF1_MODE512 
        if (font->fontMode == PSF1_MODE512) numGlyphs = 512;


        // Parse glyphs
        char *s = font + 1;
        // TODO: Implement this.
    } else if (font2->magic[0] == 0x72 && font2->magic[1] == 0xB5 && font2->magic[2] == 0x4A && font2->magic[3] == 0x86) {
        // This is a PSF 2 file.
        psfVer = 2;

        psfParseUnicode2(font2);

    } else {
        psfVer = -1;
        serialPrintf("Found an unknown PSF font.\n");
        return;
    }
}



// psfDrawChar(unsigned short int c, int cx, int cy, uint32_t fg, uint32_t bg) - Draw a PC screen font character.
void psfDrawChar(unsigned short int c, int cx, int cy, uint32_t fg, uint32_t bg) {
    if (psfVer != 2) {
        serialPrintf("Tried to use not yet supported version.\n");
        return;
    }

    PSF2_Header *font = (PSF2_Header*)&_binary_source_fonts_font_psf_start;

    // Check how many bytes encode one row.
    int bytesPerLine = (font->width + 7) / 8;

    // Unicode translation.
    if (psf_unicode != NULL) {
        c = psf_unicode[c];
    }

    // Get a glyph for the character.
    unsigned char *glyph = (unsigned char*)&_binary_source_fonts_font_psf_start + font->header_size + (c > 0 && c < font->glyphs ? c : 0) * font->bytesPerGlyph;

    // Calculate offset.
    int scanline = modeWidth;
    int offset = (cy * font->height * scanline) + (cx * (font->width + 1) * sizeof(uint32_t));
    serialPrintf("cy = %i, height = %i, scanline = %i, cx = %i, width = %i, size = %i\n", cy, font->height, scanline, cx, font->width, sizeof(uint32_t));
    // Display the pixels according to the bitmap.
    int x, y, line, mask;
    for (y = 0; y < font->height; y++) {
        line = offset;
        mask = 1 << (font->width - 1);
        serialPrintf("x = %i, y = %i, line = %i, mask = %i\n", x, y, line, mask);
        // Display a row.
        for (x = 0; x < font->width; x++) {
            *((uint32_t*)(vbeBuffer + line)) = *((unsigned int*)glyph) & mask ? fg : bg;
            mask >>= 1;
            line += sizeof(uint32_t);
        }

        // Adjust to next line
        glyph += bytesPerLine;
        offset += scanline;
    }
}