// ===================================================================
// gfx.c - Graphics functions for drawing and such.
// ===================================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/gfx.h> // Main header file



// gfxDrawRect(int x1, int y1, int x2, int y2, uint32_t color, bool fill) - Draws a rectangle.
void gfxDrawRect(int x1, int y1, int x2, int y2, uint32_t color, bool fill) {
    // Begin by drawing the first top and bottom lines.
    if ((x2-x1) > 0) {
        for (int i = x1; i < x2; i++) {
            vbePutPixel(i, y1, color);
            vbePutPixel(i, y2, color);
        }
    } else if ((x2-x1) < 0) {
        for (int i = x2; i > x1; i--) {
            vbePutPixel(i, y1, color);
            vbePutPixel(i, y2, color);
        }
    }

    // Now, draw the left and right line.
    if ((y2-y1) > 0) {
        for (int i = y1; i < y2; i++) {
            vbePutPixel(x1, i, color);
            vbePutPixel(x2, i, color);
        }
    } else if ((y2-y1) < 0) {
        for (int i = y2; i > y1; i--) {
            vbePutPixel(x1, i, color);
            vbePutPixel(x2, i, color);
        }
    }

    if (fill) {
        if ((y2-y1) > 0) {
            for (int y = y1; y < y2; y++) {
                if ((x2-x1) > 0) {
                    for (int x = x1; x < x2; x++) {
                        vbePutPixel(x, y, color);
                    }
                } else if ((x2-x1) < 0) {
                    for (int x = x2; x > x1; x--) {
                        vbePutPixel(x, y, color);
                    }
                }
            }
        } else if ((y2-y1) < 0) {
            for (int y = y2; y > y1; y--) {
                if ((x2-x1) > 0) {
                    for (int x = x1; x < x2; x++) {
                        vbePutPixel(x, y, color);
                    }
                } else if ((x2-x1) < 0) {
                    for (int x = x2; x > x1; x--) {
                        vbePutPixel(x, y, color);
                    }
                }
            }
        }
    }
}


// gfxDrawLine(int x1, int y1, int x2, int y2, uint32_t color) - Draw a line.
void gfxDrawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    // Determine direction to draw the line in.
    int dx = (x1 < x2) ? 1 : -1;
    int dy = (y1 < y2) ? 1 : -1;
    
    // Calculate the slope of the line.
    int slope = (y2-y1) / (x2-x1);

    // Start drawing.
    int x = x1;
    int y = y1;

    if (slope < 1) {
        int error = abs((x2-x1)/2);

        while (x != x2) {
            vbePutPixel(x, y, color);

            x += dx;
            error -= abs((y2-y1));
            if (error < 0) {
                y += dy;
                error += abs((x2-x1));
            }
        }
    } else if (slope > 1) {
        int error = abs((y2-y1)/2);
        
        while (y != y2) {
            vbePutPixel(x, y, color);

            y += dy;
            error -= abs((x2-x1));
            if (error < 0) {
                x += dx;
                error += abs((y2-y1));
            }
        }
    } else if (slope == 1) {
        while (x != x2 && y != y2) {
            vbePutPixel(x, y, color);

            x += dx;
            y += dy;
        }
    }
}

