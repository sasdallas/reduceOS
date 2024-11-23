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

typedef void (*putchar_t)(char ch, int x, int y, uint8_t color);
typedef void (*putpixel_t)(int x, int y, uint32_t color);
typedef void (*updcursor_t)(size_t x, size_t y);
typedef void (*clearscreen_t)(uint8_t fg, uint8_t bg);
typedef void (*updscreen_t)();

typedef struct _video_driver {
    // Driver information
    char            name[64];
    
    // Information/fields of the video driver
    // Optional, you can fill these fields with anything - these are to help the video driver.
    uint32_t    screenWidth;            // Width
    uint32_t    screenHeight;           // Height
    uint32_t    screenPitch;            // Pitch
    uint32_t    screenBPP;              // BPP
    uint8_t     *videoBuffer;           // Video buffer
    int         allowsGraphics;         // Whether it allows graphics (WARNING: This may be used. It is best to leave this correct!)

    // Functions
    putchar_t       putchar;
    putpixel_t      putpixel;
    updcursor_t     cursor;
    clearscreen_t   clear;
    updscreen_t     update;

    // Font information - this must be set
    size_t          fontWidth;
    size_t          fontHeight;
} video_driver_t;


#endif