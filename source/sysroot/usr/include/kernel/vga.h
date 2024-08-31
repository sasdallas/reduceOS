// vga.h - A collection of VGA/graphics related functions such as monitor hardware detection

#ifndef VGA_H
#define VGA_H

// Typedefs
typedef enum monitor_video_type {
    VIDEO_TYPE_NONE = 0x00,
    VIDEO_TYPE_COLOR = 0x20,
    VIDEO_TYPE_MONOCHROME = 0x30,
};


// Definitions
#define BIOS_DETECTED_HARDWARE_OFFSET 0x10 // Refers to an offset in the BDA (https://wiki.osdev.org/Memory_Map_(x86)#BIOS_Data_Area_.28BDA.29)
                                           // Should be moved to HAL 


// Functions
enum monitor_video_type monitor_getVideoType();

#endif
