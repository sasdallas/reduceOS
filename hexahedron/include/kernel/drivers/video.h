/**
 * @file hexahedron/include/kernel/drivers/video.h
 * @brief Generic video driver header
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_VIDEO_H
#define DRIVERS_VIDEO_H

/**** INCLUDES ****/
#include <stdint.h>
#include <stddef.h>

/**** TYPES ****/

// This type is for any color. You can use the definitions or macros to interact, or make your own.
typedef union _color {
    unsigned long rgb;
    struct {
        unsigned char b;
        unsigned char g;
        unsigned char r;
    } c;
} color_t;

struct _video_driver; // Prototype

typedef void (*putpixel_t)(struct _video_driver *driver, int x, int y, color_t color); // Place a pixel
typedef void (*clearscreen_t)(struct _video_driver *driver, color_t fg, color_t bg); // Clear the screen
typedef void (*updscreen_t)(struct _video_driver *driver); // Update the screen if double-buffering is used

typedef struct _video_driver {
    // Driver information
    char            name[64];
    
    // Information/fields of the video driver
    // Optional, you can fill these fields with anything - these are to help the video driver.
    uint32_t        screenWidth;            // Width
    uint32_t        screenHeight;           // Height
    uint32_t        screenPitch;            // Pitch
    uint32_t        screenBPP;              // BPP
    uint8_t         *videoBuffer;           // Video buffer
    int             allowsGraphics;         // Whether it allows graphics (WARNING: This may be used. It is best to leave this correct!)

    // Functions
    putpixel_t      putpixel;
    clearscreen_t   clear;
    updscreen_t     update;

    // Fonts and other information will be handled by the font driver
} video_driver_t;

/**** MACROS ****/

// RGB manipulation functions
#define RGB_R(color) (color.c.r & 255)
#define RGB_G(color) (color.c.g & 255)
#define RGB_B(color) (color.c.b & 255)
#define RGB(red, green, blue) (color_t){.c.r = red, .c.g = green, .c.b = blue}


/**** DEFINITIONS ****/

// Defines for VGA text mode graphics converted to RGB
#define COLOR_BLACK             RGB(0, 0, 0)
#define COLOR_BLUE              RGB(0, 0, 170)
#define COLOR_GREEN             RGB(0, 170, 0)
#define COLOR_CYAN              RGB(0, 170, 170)
#define COLOR_RED               RGB(170, 0, 0)
#define COLOR_PURPLE            RGB(170, 0, 170)
#define COLOR_BROWN             RGB(170, 85, 0)
#define COLOR_GRAY              RGB(170, 170, 170)
#define COLOR_DARK_GRAY         RGB(85, 85, 85)
#define COLOR_LIGHT_BLUE        RGB(85, 85, 255)
#define COLOR_LIGHT_GREEN       RGB(85, 255, 85)
#define COLOR_LIGHT_CYAN        RGB(85, 255, 255)
#define COLOR_LIGHT_RED         RGB(255, 85, 85)
#define COLOR_LIGHT_PURPLE      RGB(255, 85, 255)
#define COLOR_YELLOW            RGB(255, 255, 85)
#define COLOR_WHITE             RGB(255, 255, 255)

/**** FUNCTIONS ****/



#endif