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

// External variables
extern char _binary_font_psf_start;
extern char _binary_font_psf_end;



// Functions

// reduceOS loads in with a default font for PSF, encoded into the actual kernel binary file.
// Other PSF fonts can be loaded from the initial ramdisk or storage mediums.


// psfInit() - Initializes the default PSF font for reduceOS.
void psfInit() {

}


