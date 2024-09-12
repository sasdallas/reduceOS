// video.h - Video driver for the reduceOS kernel

#ifndef VIDEO_H
#define VIDEO_H

// Includes
#include <libk_reduced/stdint.h>
#include <kernel/hashmap.h>

// Typedefs

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

    // Screen information
    video_driver_info_t *info;
} video_driver_t;



// Functions
void video_init();
void video_setcolor(uint8_t f, uint8_t b);
void video_putchar(char c, int x, int y, uint8_t color);
void video_clearScreen(uint8_t fg, uint8_t bg);
video_driver_info_t *video_getInfo();
void video_update_screen();
void video_cursor(size_t x, size_t y);
void video_putpixel(int x, int y, uint32_t color);
void video_change();

void video_registerDriver(video_driver_t *driver, int isOptimal);
video_driver_t *video_getDriver(char *drivername);
video_driver_t *video_getCurrentDriver();

size_t video_getFontWidth();
size_t video_getFontHeight();
uint32_t video_getScreenWidth();
uint32_t video_getScreenHeight();
int video_canHasGraphics();



#endif