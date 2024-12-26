/**
 * @file hexahedron/misc/term.c
 * @brief Terminal driver for Hexahedron
 * 
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/gfx/term.h>
#include <stddef.h>

/* Variables */
int terminal_width = 0;
int terminal_height = 0;
int terminal_x = 0;
int terminal_y = 0;
color_t terminal_fg = TERMINAL_DEFAULT_FG;
color_t terminal_bg = TERMINAL_DEFAULT_BG;

/**
 * @brief Initialize the terminal system - sets terminal_print as default printf method.
 * @returns 0 on success, -EINVAL on no video driver/font driver.
 * @note The terminal can always be reinitalized. It will clear the screen and reset parameters.
 */
int terminal_init(color_t fg, color_t bg) {
    // Get video information
    video_driver_t *driver = video_getDriver();
    if (!driver) return -EINVAL; // No video driver available

    // Get font data
    int fontHeight = font_getHeight();
    int fontWidth = font_getWidth();
    if (!fontHeight || !fontWidth) return -EINVAL; // No font loaded

    // Setup terminal variables
    terminal_width = driver->screenWidth / fontWidth;
    terminal_height = driver->screenHeight / fontHeight;
    terminal_fg = fg;
    terminal_bg = bg;

    // Reset terminal X and terminal Y
    terminal_x = terminal_y = 0;

    // Clear screen
    terminal_clear(terminal_fg, terminal_bg);

    // Done!
    return 0;
}

/**
 * @brief Clear terminal screen
 * @param fg The foreground of the terminal
 * @param bg The background of the terminal
 */
void terminal_clear(color_t fg, color_t bg) {
    terminal_fg = fg;
    terminal_bg = bg;

    for (int y = 0; y < terminal_height; y++) {
        for (int x = 0; x < terminal_width; x++) {
            font_putCharacter(' ', x, y, terminal_fg, terminal_bg);
        }
    }
}

/**
 * @brief Handle a backspace in the terminal
 */
static void terminal_backspace() {
    terminal_x--;
    terminal_putchar(' ');
    terminal_x--;
}

/**
 * @brief Put character method
 */
int terminal_putchar(int c) {
    if (!terminal_width || !terminal_height) return 0;

    // Handle the character
    switch (c) {
        case '\n':
            // Newline
            terminal_x = 0;
            terminal_y++;
            break;
    
        case '\b':
            // Backspace
            terminal_backspace();
            break;
        
        case '\0':
            // Null character
            break;
        
        case '\t':
            // Tab
            for (int i = 0; i < 4; i++) terminal_putchar(' ');
            break;
        
        case '\r':
            // Return carriage
            terminal_x = 0;
            break;
        
        default:
            // Normal character
            font_putCharacter(c, terminal_x, terminal_y, terminal_fg, terminal_bg);
            terminal_x++;
            break;
    }

    if (terminal_x >= terminal_width) {
        // End of the screen width
        terminal_y++;
        terminal_x = 0;
    }

    // Clear the screen if the height is too big
    if (terminal_y >= terminal_height) {
        terminal_clear(terminal_fg, terminal_bg); // No scrolling yet
    }

    return 0;
}

/**
 * @brief Put character method (printf-conforming)
 */
int terminal_print(void *user, int c) {
    return terminal_putchar(c);
}

/**
 * @brief Set the coordinates of the terminal
 */
void terminal_setXY(int x, int y) {
    if (x >= terminal_width) return;
    if (y >= terminal_height) return;
    terminal_x = x;
    terminal_y = y;
}


/**** GETTER FUNCTIONS ****/

/**
 * @brief Get the current X of the terminal
 */
int terminal_getX() {
    return terminal_x;
}

/**
 * @brief Get the current Y of the terminal
 */
int terminal_getY() {
    return terminal_y;
}

/**
 * @brief Get the current fg of the terminal
 */
color_t terminal_getForeground() {
    return terminal_fg;
}

/**
 * @brief Get the current bg of the terminal
 */
color_t terminal_getBackground() {
    return terminal_bg;
}

/**
 * @brief Get the current width of the terminal
 */
int terminal_getWidth() {
    return terminal_width;
}

/**
 * @brief Get the current height of the terminal
 */
int terminal_getHeight() {
    return terminal_height;
}