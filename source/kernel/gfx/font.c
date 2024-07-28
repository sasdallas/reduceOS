// ==========================================================
// font.c - reduceOS font parser (supports bitmap and PSF)
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include "include/font.h" // Main header file

// PSF (as mentioned in the header file) stands for 'PC screen font', which is the font used by Linux for its console.
// The font is encoded with its own header and in unicode
// You can find more information on it here: https://wiki.osdev.org/PC_Screen_Font
// Unicode table decoding here is also sourced from the above link.

PSF2_Header *font; // TODO: PSF1 support

// External variables
extern char _binary_source_fonts_font_psf_start;
extern char _binary_source_fonts_font_psf_end;
extern uint32_t *vbeBuffer;
extern uint32_t font_data[127][20];

uint32_t currentFont[127][20];


// Bitmap Font Functions

// bitmapFontInit() - Initializes bitmap font reading with the default font found in font_data.c
void bitmapFontInit() {
    memcpy(currentFont, font_data, sizeof(font_data));
}

// bitmapLoadFont(uint32_t *font_data) - Loads a a new font for the bitmap.
void bitmapLoadFont(uint32_t *font_data) {
    memcpy(currentFont, font_data, sizeof(currentFont));
}




// bitmapFontDrawChar(char ch, int x, int y, int color) - Puts a character of color 'color' at x, y
void bitmapFontDrawChar(char ch, int x, int y, int color) {
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

// bitmapFontDrawString(char *str, int x, int y, int color) - Draw a string from the bitmap
void bitmapFontDrawString(char *str, int x, int y, int color) {
    int tmpX = x, tmpY = y;
    while (*str != '\0') {
        bitmapFontDrawChar(*str, tmpX, tmpY, color);
        *str++;
        tmpX += 14;

        if (tmpX > modeWidth) {
            tmpY += 17;
            tmpX = x;
        }
    }
}



// PSF Functions

//uint8_t *psf_data;

// reduceOS loads in with a default font for PSF, encoded into the actual kernel binary file.
// Other PSF fonts can be loaded from the initial ramdisk or storage mediums.


void psfGetPSFInfo() {
    serialPrintf("psfGetPSFInfo: PSF font is loaded from 0x%x to 0x%x\n", &_binary_source_fonts_font_psf_start, &_binary_source_fonts_font_psf_end);
    char *start_addr = &_binary_source_fonts_font_psf_start;
    PSF1_Header *h1 = start_addr;
    if (h1->magic == 0x0436) {
        serialPrintf("psfGetPSFInfo: Font is PSF version 1\n");
        serialPrintf("psfGetPSFInfo: Font mode: %i\npsfGetPSFInfo: Character Size: %i\n", h1->fontMode, h1->characterSize);
    }


    PSF2_Header *h2 = start_addr;
    if (h2->magic == 0x864ab572) {
        serialPrintf("psfGetPSFInfo: Font is PSF version 2\n");
        serialPrintf("psfGetPSFInfo: Version is %i\n", h2->version);
        serialPrintf("psfGetPSFInfo: Header size: %i\n",h2->header_size);
        serialPrintf("psfGetPSFInfo: Unicode table contained: %s\n", (h2->flags == 0) ? "No" : "Yes");
        serialPrintf("psfGetPSFInfo: Glyphs = %i (size = %i bytes)\n", h2->glyphs, h2->bytesPerGlyph);
        serialPrintf("psfGetPSFInfo: height = %i width = %i\n", h2->height, h2->width);
    }

}


// psfInit() - Initializes the default PSF font for reduceOS.
void psfInit() {
    char *start_addr = &_binary_source_fonts_font_psf_start;


    PSF1_Header *h1 = start_addr;
    if (h1->magic == 0x0436) {
        serialPrintf("psfInit: PSF version 1 is NOT supported!\n");
        return;
    }


    PSF2_Header *h2 = start_addr;
    if (h2->magic == 0x864ab572) {
        font = h2;
    }
    
    printf("PSF initialized\n");
}


int psfGetFontWidth() { return font->width; }
int psfGetFontHeight() { return font->height; }

// psfDrawChar(unsigned short int c, int cx, int cy, uint32_t fg, uint32_t bg) - Draw a PC screen font character.
void psfDrawChar(unsigned short int c, int cx, int cy, uint32_t fg, uint32_t bg) {
    // Check how many bytes encode one row.
    int bytesPerLine = (font->width + 7) / 8;
    
    // Get a glyph for the character.
    unsigned char *glyph = (unsigned char*)&_binary_source_fonts_font_psf_start + font->header_size + (c > 0 && c < font->glyphs ? c : 0) * font->bytesPerGlyph;

    // Display the pixels according to the bitmap.
    int x, y, mask;
    for (y = 0; y < font->height; y++) {
        mask = 1 << (font->width - 1);
        // Display a row.
        for (x = 0; x < font->width; x++) {
            if ((*((unsigned int*)glyph) & mask ? fg : bg) == bg) {
                vbePutPixel(cx + x, cy + y, bg);
            } else {
                vbePutPixel(cx + x, cy + y, fg);
            }
            mask >>= 1;
        }

        // Adjust to next line
        glyph += bytesPerLine;
    }
}

