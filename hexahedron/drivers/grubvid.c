/**
 * @file hexahedron/drivers/grubvid.c
 * @brief Generic LFB/GRUB video driver
 * 
 * This isn't a driver model, it's just a video driver that can work with a framebuffer
 * passed by GRUB (unless its EGA, blegh.)
 * Also, this is better known as an LFB video driver, since that's what GRUB passes, but whatever.
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <kernel/drivers/grubvid.h>
#include <kernel/drivers/video.h>
#include <kernel/mem/alloc.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/pmm.h>
#include <kernel/debug.h>
#include <string.h>

/* Log method */
#define LOG(status, message, ...) dprintf_module(status, "GRUBVID", message, ## __VA_ARGS__)

/**
 * @brief Put pixel function
 */
void grubvid_putPixel(video_driver_t *driver, int x, int y, color_t color) {
    uintptr_t location = x * 4 + y * driver->screenPitch;

    driver->videoBuffer[location] = RGB_B(color);
    driver->videoBuffer[location + 1] = RGB_G(color);
    driver->videoBuffer[location + 2] = RGB_R(color);
}

/**
 * @brief Clear screen function
 */
void grubvid_clearScreen(video_driver_t *driver, color_t bg) {
    // fg is ignored, fill screen with bg
    uint8_t *buffer = driver->videoBuffer;
    for (uint32_t y = 0; y < driver->screenHeight; y++) {
        for (uint32_t x = 0; x < driver->screenWidth; x++) {
            buffer[x*4] = RGB_B(bg);
            buffer[x*4+1] = RGB_G(bg);
            buffer[x*4+2] = RGB_R(bg);  
        }

        buffer += driver->screenPitch;
    }
}

/**
 * @brief Update screen function
 */
void grubvid_updateScreen(struct _video_driver *driver) {
    // No double buffering
}

/**
 * @brief Communication function. Allows for ioctl on video-specific drivers.
 * 
 * See @c grubvid.c for specific calls
 */
int grubvid_communicate(struct _video_driver *driver, int type, uint32_t *data) {
    // Unimplemented
    return 0;
}

/**
 * @brief Initialize the GRUB video driver
 * @param parameters Generic parameters containing the LFB driver.
 * @returns NULL on failure to initialize, else a video driver structure
 */
video_driver_t *grubvid_initialize(generic_parameters_t *parameters) {
    if (!parameters || !parameters->framebuffer) return NULL;
    if (!parameters->framebuffer->framebuffer_addr) return NULL;

    video_driver_t *driver = kmalloc(sizeof(video_driver_t));
    
    strcpy((char*)driver->name, "GRUB Video Driver");

    driver->screenWidth = parameters->framebuffer->framebuffer_width;
    driver->screenHeight = parameters->framebuffer->framebuffer_height;
    driver->screenPitch = parameters->framebuffer->framebuffer_pitch;
    driver->screenBPP = parameters->framebuffer->framebuffer_bpp;
    driver->allowsGraphics = 1;

    driver->putpixel = grubvid_putPixel;
    driver->clear = grubvid_clearScreen;
    driver->update = grubvid_updateScreen;
    driver->communicate = grubvid_communicate;

    // BEFORE WE DO ANYTHING, WE HAVE TO REMAP THE FRAMEBUFFER TO SPECIFIED ADDRESS
    for (uintptr_t phys = parameters->framebuffer->framebuffer_addr, virt = MEM_FRAMEBUFFER_REGION;
            phys < parameters->framebuffer->framebuffer_addr + ((driver->screenWidth * driver->screenHeight) * 4);
            phys += PAGE_SIZE, virt += PAGE_SIZE) 
    {
        mem_mapAddress(NULL, phys, virt, MEM_KERNEL); // !!!: usermode access?
    }

    driver->videoBuffer = (uint8_t*)MEM_FRAMEBUFFER_REGION;

    return driver;
}
