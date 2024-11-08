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

// This structure should be returned by the getinfo_t function
typedef struct _video_driver_info_t {
    uint32_t    screenWidth;
    uint32_t    screenHeight;
    uint32_t    screenPitch;
    uint32_t    screenBPP;
    uint8_t     *videoBuffer;
    int         allowsGraphics;
} video_driver_info_t;

typedef void (*putchar_t)(char ch, int x, int y, uint8_t color);
typedef void (*putpixel_t)(int x, int y, uint32_t color);
typedef void (*updcursor_t)(size_t x, size_t y);
typedef void (*clearscreen_t)(uint8_t fg, uint8_t bg);
typedef void (*updscreen_t)();
typedef video_driver_info_t* (*getinfo_t)();

typedef struct _video_driver_t {
    // Driver information
    char            name[128];
    
    // Functions
    getinfo_t       getinfo;
    putchar_t       putchar;
    putpixel_t      putpixel;
    updcursor_t     cursor;
    clearscreen_t   clear;
    updscreen_t     update;

    // Font information
    size_t          fontWidth;
    size_t          fontHeight;
} video_driver_t;


#endif