// graphics.h - Header file for graphics.c
// Note that graphics.c and terminal.c are different. One houses the main code for anything graphics related(drawing, colors, etc) and the other handles console output and misc. other functions.
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "include/libc/stdint.h" // Integer declarations
#include "include/libc/stdbool.h" // Boolean declarations
#include "include/libc/stddef.h" // size_t declaration
#include "include/libc/string.h" // String functions


// Basic graphics definitions, like the video memory address
static uint16_t* const VIDEO_MEM = (uint16_t*) 0xB8000;
static size_t SCREEN_WIDTH = 80;
static size_t SCREEN_HEIGHT = 25; 

// Color enumerator.
enum gfxColor {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
};

// vgaEntry() - Functions returns a valid VGA character entry.
// Parameters: unsigned char uc and uint8_t color
static inline uint16_t vgaEntry(unsigned char uc, uint8_t color) { return (uint16_t) uc | (uint16_t) color << 8; }

// vgaColorEntry() - Function returns a valid VGA color entry.
// Parameters: Two gfxColor enums, foreground color and background color
static inline uint8_t vgaColorEntry(enum gfxColor foreground, enum gfxColor background) { return foreground | background << 4; }


#endif