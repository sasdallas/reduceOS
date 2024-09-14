/**
 * @file source/kernel/kernel/doom.c
 * @brief ;)
 */

// This file will provide stuff for doomgeneric, which is hosted by ozkl. These functions can be accessed via extern calls.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <kernel/fb.h>

#include "i_video.h"
#include "i_system.h"

extern uint32_t *DG_ScreenBuffer;

static uint8_t *framebuffer = 0x0;
static int framebuffer_fd = -1;
static long width, height, depth;

static void set_point(int x, int y, uint32_t value) {
	uint32_t * disp = (uint32_t *)framebuffer;
	uint32_t * cell = &disp[y * width + x];
	*cell = value;
}


static void clear_screen(void) {
	if (framebuffer_fd < 0) return;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			set_point(x,y,0xFF050505);
		}
	}
}


void DG_Init() {
    // Setup our console, as usual.
    open("/device/console", 0);
    open("/device/console", 0);
    open("/device/console", 0); // forced

    // Time to get graphics running.
    // The framebuffer device is located at /device/fb0
    framebuffer_fd = open("/device/fb0", O_RDONLY);

    if (framebuffer_fd < 0) {
        printf("*** Launch failed. Please check that the framebuffer works!\n");
        for (;;);
    }

    // Use ioctl to get all the information
    ioctl(framebuffer_fd, FBIOGET_SCREENW, &width);
    ioctl(framebuffer_fd, FBIOGET_SCREENH, &height);
    ioctl(framebuffer_fd, FBIOGET_SCREENDEPTH, &depth);
    ioctl(framebuffer_fd, FBIOGET_SCREENADDR, &framebuffer);
    
    printf("Successfully initialized framebuffer!\n");
    printf("DG_BUFFER = 0x%x\nFramebuffer = 0x%x\n", DG_ScreenBuffer, framebuffer);

    clear_screen();

    printf("Initializing DOOM...\n");
}


void DG_DrawFrame() {
    int i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            set_point(x, y, DG_ScreenBuffer[i]);
            i++;
        }
    }
}


void DG_SleepMs(uint32_t ms) {
    I_Error("Unimplemented function DG_SleepMs");
    return;
}

uint32_t DG_GetTicksMs() {
    I_Error("Unimplemented function DG_GetTicksMS");
    __builtin_unreachable();
}

int DG_GetKey(int *pressed, unsigned char *key) {
    I_Error("Unimplemented function DG_GetKey");
    __builtin_unreachable();
}

void DG_SetWindowTitle(const char *title) {
    I_Error("Unimplemented function SetWindowTitle");
}

