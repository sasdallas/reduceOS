/**
 * @file source/kernel/gfx/video.c
 * @brief The main video driver for reduceOS
 * 
 * This driver handles interfacing with all video drivers in reduceOS. It supports multiple modes built-in.
 * 
 * Modes supported (listed in their hierarchical order):
 * - GRUB UEFI GOP framebuffer (disregarded if kargs)
 * - VESA VBE standards (disregarded if kargs)
 * - VGA Text Mode (can be forced with --force_vga)
 * - Headless mode (can be forced with --headless)
 * 
 * A video driver may be loaded via the modules system - it can call the video_registerDriver function.
 * 
 * @todo This file should probably not use uint8_t for colors when printing characters.
 * 
 * @copyright
 * This file is part of the reduceOS C kernel, which is licensed under the terms of GPL.
 * Please credit me if this code is used.
 * 
 * Copyright (C) 2024 Samuel S.
 */

#include <kernel/video.h>
#include <kernel/font.h>
#include <kernel/args.h>
#include <kernel/vesa.h>
#include <kernel/hashmap.h>


static hashmap_t *video_driver_hashmap;
static video_driver_t *currentDriver;
extern multiboot_info *globalInfo;

static uint8_t fg;
static uint8_t bg;

void vga_putchar(char c, int x, int y, uint8_t color);
void vga_putpixel(int x, int y, uint32_t color);
void vga_updateTextCursor(size_t y, size_t x);
video_driver_info_t *vga_get_info();
void vga_clearscreen(uint8_t fg, uint8_t bg);

video_driver_t *headless_getDriver();
video_driver_t *vga_getDriver();
video_driver_t *vesa_getDriver(int useGOP);

extern int VESA_Initialized;


// video_init() - Initialize the video subsystem
void video_init() {
    // We should first setup the hashmap
    video_driver_hashmap = hashmap_create(10);

    // Let's figure out what we want to do. 
    // The current hierarchical list enables forcing VGA text mode and disregarding all other options, but just in case, go normal.

    // Headless mode? You bet!
    if (args_has("--headless")) {
        // Force headless (serial communication) mode
        video_driver_t *headless = headless_getDriver();

        hashmap_set(video_driver_hashmap, headless->name, headless);
        currentDriver = headless;
        return;
    }

    // Some computers (most) don't support our VESA VBE driver.
    // We support the kernel argument --force_vga which will force VGA text mode, which half-works
    if (args_has("--force_vga")) {
        // Force VGA text mode
        video_driver_t *vga_mode = vga_getDriver();

        hashmap_set(video_driver_hashmap, "VGA Text Mode", vga_mode);
        currentDriver = vga_mode;
        return;
    }


    // Else, let's go through and see what we got.

    // Let's try to use GOP mode from GRUB.
    if (globalInfo->framebuffer_addr) {
        // First, what we're going to do might seem a little weird, but it should work.
        // Map the framebuffer to 0xFD000000. This is usually the classic VBE address used - and GRUB will normally use this anyways.
        // GCC will not accept int-to-pointer casts, and mapping it to a predefined kernel location is probably better regardless.
        for (uint32_t i = 0; i < (globalInfo->framebuffer_width * globalInfo->framebuffer_height) * 4; i += 0x1000) vmm_allocateRegion(globalInfo->framebuffer_addr + i, (0xFD000000 + i), 0x1000);

        VESA_Initialized = 1;
        vbeBuffer   = (uint8_t*)0xFD000000;
        modeWidth   = globalInfo->framebuffer_width;
        modeHeight  = globalInfo->framebuffer_height;
        modeBpp     = globalInfo->framebuffer_bpp;
        modePitch   = globalInfo->framebuffer_pitch;
        framebuffer = kmalloc(globalInfo->framebuffer_width * globalInfo->framebuffer_height * 4);

        video_driver_t *gop_mode = vesa_getDriver(1); // Little bit hacky.
        currentDriver = gop_mode;
        hashmap_set(video_driver_hashmap, gop_mode->name, gop_mode);

        return;
    }

    // Let's try to initialize VESA VBE mode.
    video_driver_t *vbe_mode = vesa_getDriver(0); 
    currentDriver = vbe_mode;
    hashmap_set(video_driver_hashmap, vbe_mode->name, vbe_mode);

}

void video_putchar(char c, int x, int y, uint8_t color) {
    if (currentDriver->putchar) {
        currentDriver->putchar(c, x, y, color);
    }
}

void video_putpixel(int x, int y, uint32_t color) {
    if (currentDriver->putpixel) {
        currentDriver->putpixel(x, y, color);
    }
}

void video_cursor(size_t x, size_t y) {
    if (currentDriver->cursor) {
        currentDriver->cursor(x, y);
    }
}

void video_setcolor(uint8_t f, uint8_t b) {
    fg = f;
    bg = b;
}

void video_update_screen() {
    if (currentDriver->update) {
        currentDriver->update();
    }
}


video_driver_info_t *video_getInfo() {
    if (currentDriver->getinfo) {
        return currentDriver->getinfo();
    } else {
        return NULL;
    }
}

void video_clearScreen(uint8_t f, uint8_t b) {
    if (currentDriver->clear) {
        currentDriver->clear(f, b);
    }
}

size_t video_getFontWidth() {
    return currentDriver->fontWidth;
}

size_t video_getFontHeight() {
    return currentDriver->fontHeight;
}

uint32_t video_getScreenWidth() {
    if (currentDriver->info) {
        return currentDriver->info->screenWidth;
    } else {
        return 0;
    }
}

uint32_t video_getScreenHeight() {
    if (currentDriver->info) {
        return currentDriver->info->screenHeight;
    } else {
        return 0;
    }
}

int video_canHasGraphics() {
    if (currentDriver->info) {
        return currentDriver->info->allowsGraphics;
    } else {
        return 0;
    }
}

void video_change() {
    // This function is used to notify the current video driver of a change.
    // We will reacquire it's information and store it.
    if (currentDriver->getinfo) {
        video_driver_info_t *info = currentDriver->getinfo();
        kfree(currentDriver->info);
        currentDriver->info = info;
    }
}

/** VIDEO DRIVER REGISTRATION **/

void video_registerDriver(video_driver_t *driver, int isOptimal) {
    // WARNING: THIS SYSTEM IS LIKELY BAD.
    if (!driver) return;

    hashmap_set(video_driver_hashmap, driver->name, driver);
    if (isOptimal) {
        serialPrintf("video: Changing the current video driver (%s) to a new optimal one - %s\n", currentDriver->name, driver->name);
        currentDriver = driver;
        
        // Reinitialize the terminal
        if (video_canHasGraphics(driver)) changeTerminalMode(1);
        else changeTerminalMode(0);
        initTerminal();
    }
}

video_driver_t *video_getDriver(char *drivername) {
    return (video_driver_t*)hashmap_get(video_driver_hashmap, drivername);
}

video_driver_t *video_getCurrentDriver() {
    return currentDriver;
}

/** VESA OUTPUT FUNCTIONS (TO BE MOVED TO EXISTING VESA CODE) */

void vesa_putchar(char c, int x, int y, uint8_t color) {
    psfDrawChar(c, x, y, VGA_TO_VBE(color), VGA_TO_VBE(bg)); // Perhaps there is a better way to do this
}

video_driver_info_t *vesa_get_info() {
    video_driver_info_t *out = kmalloc(sizeof(video_driver_info_t));
    out->allowsGraphics = 1;
    out->screenBPP = modeBpp;
    out->screenWidth = modeWidth;
    out->screenHeight = modeHeight;
    out->screenPitch = modePitch;
    out->videoBuffer = (uint8_t*)framebuffer; // should implement a better double buffering way of doing this
    return out;
}

void vesa_putpixel(int x, int y, uint32_t color) {
    vbePutPixel(x, y, color);
}

void vesa_cursor(size_t x, size_t y) {
    updateTextCursor_vesa();
}

void vesa_clearscreen(uint8_t f, uint8_t b) {
    video_setcolor(f, b);
    for (uint32_t y = 0; y < modeHeight; y++) {
        for (uint32_t x = 0; x < modeWidth; x++) {
            // bad code fix later
            vbePutPixel(x, y, VGA_TO_VBE(b));
        }
    }
    vbeSwitchBuffers();
}

void vesa_update() {
    vbeSwitchBuffers();
}

// VESA initialization function
video_driver_t *vesa_getDriver(int useGOP) {
    // This serves as our initialization function too!
    // useGOP specifies whether a framebuffer was setup for us, because the functions are the same.
    // If useGOP was 1 and vesaInit fails, it did so because our code pre-emptively setup a framebuffer for it.
    // We could also check the error code the function fails at.

    psfInit();

    if (vesaInit() && !useGOP) {
        // No luck - return NULL.
        return NULL;
    }

    changeTerminalMode(1);

    video_driver_t *driver = kmalloc(sizeof(video_driver_t));
    driver->putpixel    = vesa_putpixel;
    driver->cursor      = vesa_cursor;
    driver->getinfo     = vesa_get_info;
    driver->update      = vesa_update;
    driver->putchar     = vesa_putchar;
    driver->clear       = vesa_clearscreen;
    driver->fontHeight  = psfGetFontHeight();
    driver->fontWidth   = psfGetFontWidth();
    driver->info        = vesa_get_info();
    strcpy(driver->name, "VESA VBE Driver");
    return driver;
}


/** VGA TEXT MODE OUTPUT FUNCTIONS **/

static uint16_t *vga_buffer = (uint16_t*)0xB8000;

void vga_clearscreen(uint8_t fg, uint8_t bg) {
    for (size_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (size_t x = 0; x < SCREEN_WIDTH; x++) {
            vga_putchar(' ', x, y, vgaColorEntry(fg, bg)); // Reset all characters to be ' '. Obviously initTerminal() already does this, but we want to reimplement it without all the setup stuff.
        }
    }
}

void vga_putchar(char c, int x, int y, uint8_t color) {
    const size_t index = y * SCREEN_WIDTH + x; // Same code as in initTerminal(). Calculate index to place char at and copy it.
    vga_buffer[index] = vgaEntry(c, color);
}

void vga_putpixel(int x, int y, uint32_t color) {
    return;
}

void vga_updateTextCursor(size_t x, size_t y) {
    uint16_t pos = y * SCREEN_WIDTH + x;

    outportb(0x3D4, 14);
    outportb(0x3D5, pos >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, pos);
}



video_driver_info_t *vga_get_info() {
    video_driver_info_t *out = kmalloc(sizeof(video_driver_info_t));
    out->allowsGraphics = 0;
    out->screenBPP = 8;
    out->screenWidth = SCREEN_WIDTH;
    out->screenHeight = SCREEN_HEIGHT;
    out->screenPitch = 0;
    out->videoBuffer = (uint8_t*)vga_buffer;
    return out;
}

video_driver_t *vga_getDriver() {
    // Force VGA text mode
    changeTerminalMode(0);
    video_driver_t *vga_mode = kmalloc(sizeof(video_driver_t));
    vga_mode->cursor    = vga_updateTextCursor;
    vga_mode->putchar   = vga_putchar;
    vga_mode->putpixel  = vga_putpixel;
    vga_mode->getinfo   = vga_get_info;
    vga_mode->clear     = vga_clearscreen;
    vga_mode->info      = vga_get_info();
    vga_mode->fontHeight = 1;
    vga_mode->fontWidth = 1;
    strcpy(vga_mode->name, "VGA Text Mode");
    return vga_mode;
}

/** HEADLESS FUNCTIONS **/

int lasty = 0;

void headless_putchar(char ch, int x, int y, uint8_t color) {
    // The terminal driver works by updating its own Y value
    if (y > lasty) {
        serialPrintf("\r\n"); // This should also handle the return carriage for serial emulators that need it
    }

    lasty = y;

    serialWrite(NULL, ch);
}

video_driver_info_t *headless_get_info() {
    video_driver_info_t *out = kmalloc(sizeof(video_driver_info_t));
    out->allowsGraphics = 0;
    out->screenBPP = 32;
    out->screenWidth = 80;
    out->screenHeight = 25;
    out->screenPitch = 0;
    out->videoBuffer = NULL;
    return out;
}

video_driver_t *headless_getDriver() {
    changeTerminalMode(0);

    video_driver_t *driver = kmalloc(sizeof(video_driver_t));
    driver->putchar     = headless_putchar;
    driver->getinfo     = headless_get_info;
    driver->fontHeight  = 1;
    driver->fontWidth   = 1;
    driver->info        = headless_get_info();
    strcpy(driver->name, "Headless Driver");
    return driver;
}