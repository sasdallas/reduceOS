/**
 * @file hexahedron/include/kernel/drivers/font.h
 * @brief Font driver header
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#ifndef DRIVERS_FONT_H
#define DRIVERS_FONT_H

/**** INCLUDES ****/
#include <stdint.h>
#include <kernel/drivers/video.h>


/**** TYPES ****/

typedef struct _font_data {
    int type;           // The type of the font
    size_t width;       // Width of the font
    size_t height;      // Height of the font
    uint8_t *data;      // Data pointer to the font
} font_data_t;

/**** DEFINITIONS ****/

#define FONT_TYPE_BACKUP        0       // Backup font
#define FONT_TYPE_PSF           1       // PC screen font

/**** FUNCTIONS ****/

/**
 * @brief Initializes the font driver with the backup font
 */
void font_init();

/**
 * @brief Put a character to the screen
 * @param c The character
 * @param x X coordinate - this is expected as a screen coordinate
 * @param y Y coordinate - this is expected as a screen coordinate
 * @param fg The foreground color to use
 * @param bg The background color to use
 */
void font_putCharacter(int c, int x, int y, color_t fg, color_t bg);

/**
 * @brief Get font width
 */
int font_getWidth();

/**
 * @brief Get font height
 */
int font_getHeight();

#endif