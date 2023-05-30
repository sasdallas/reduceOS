// gfx.h - header file for the graphics handler

#ifndef GFX_H
#define GFX_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/vesa.h" // VESA VBE functions

// Functions
void gfxDrawRect(int x1, int y1, int x2, int y2, uint32_t color, bool fill); // Draws a rectangle
void gfxDrawLine(int x1, int y1, int x2, int y2, uint32_t color); // Draw a line.

#endif