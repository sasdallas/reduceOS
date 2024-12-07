/**
 * @file hexahedron/drivers/font.c
 * @brief Font driver (supports backup font/PSF)
 * 
 * @warning This driver is a little bit hacky and maybe too overcomplicated.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/font.h>
#include <kernel/debug.h>
#include <kernel/misc/backup-font.h>
#include <kernel/mem/alloc.h>

/* Font data */
font_data_t *current_font = NULL;

/* Log */
#define LOG(status, ...) dprintf_module(status, "FONT", __VA_ARGS__)

/**
 * @brief Initializes the font driver with the backup font
 */
void font_init() {

#ifndef KERNEL_NO_BACKUP_FONT
    current_font = kmalloc(sizeof(font_data_t));
    current_font->data = NULL;
    current_font->width = BACKUP_LARGE_FONT_CELL_WIDTH;
    current_font->height = BACKUP_LARGE_FONT_CELL_HEIGHT;
    current_font->type = FONT_TYPE_BACKUP;
#else
    LOG(WARN, "No backup font compiled into kernel");
#endif
}


/**
 * @brief Put character function (backup font)
 * 
 * Implementation of a small backup font stored within the kernel. This backup font
 * doesn't have to be compiled.
 */
void font_putCharacterBackup(int c, int x, int y, color_t fg, color_t bg) {
    uint8_t* fc = backup_large_font[c];

    for (uint8_t h = 0; h < font_getHeight(); h++) {
        for (uint8_t w = 0; w < font_getWidth(); w++) {
            if (fc[h] & (1 << (BACKUP_LARGE_FONT_MASK - w))) {
                // Foreground pixel
                video_plotPixel(x+w, y+h, fg);
            } else {
                // Background pixel
                video_plotPixel(x+w, y+h, bg);
            }
        }
    }
}


/**
 * @brief Put a character to the screen
 * @param c The character
 * @param x X coordinate - this is expected as a screen coordinate
 * @param y Y coordinate - this is expected as a screen coordinate
 * @param fg The foreground color to use
 * @param bg The background color to use
 */
void font_putCharacter(int c, int x, int y, color_t fg, color_t bg) {
    if (!current_font) return;

    switch (current_font->type) {
        case FONT_TYPE_BACKUP:
            return font_putCharacterBackup(c, x, y, fg, bg);
        case FONT_TYPE_PSF:
            return;
        default:
            return;
    }
}

/**
 * @brief Get font width
 */
int font_getWidth() {
    if (!current_font) return 0;
    return current_font->width;
}

/**
 * @brief Get font height
 */
int font_getHeight() {
    if (!current_font) return 0;
    return current_font->height;
}

