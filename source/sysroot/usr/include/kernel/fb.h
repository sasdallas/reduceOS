// fb.h - Contains definitions for the framebuffer device (/device/fbx)
// This file can be used for ports, just use the ioctl calls

#ifndef FB_H
#define FB_H

// Includes
#ifdef __KERNEL__
#include <libk_reduced/stdint.h>
#else
#include <stdint.h>
#endif

// Definitions
#define FBIOGET_SCREENW     0x4600      // Get screen width
#define FBIOGET_SCREENH     0x4601      // Get screen height
#define FBIOGET_SCREENDEPTH 0x4602      // Get screen depth   
#define FBIOGET_SCREENPITCH 0x4603      // Get screen pitch
#define FBIOPUT_SCREENINFO  0x4604      // Unused and will trap the kernel
#define FBIOGET_SCREENADDR  0x4605      // Returns the address for video memory
#define FBIOPUT_SCREENADDR  0x4606      // Unused and will trap the kernel

// Typedefs

/* TEMPORARY */
typedef uint32_t width_t;
typedef uint32_t height_t;

typedef struct {
    width_t     width;
    height_t    height;
    uint32_t    bpp;
} fb_info_t; 



#endif