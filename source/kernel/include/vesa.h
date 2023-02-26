// vesa.h - header file for the VESA VBE handler

#ifndef VESA_H
#define VESA_H

// Includes
#include "include/libc/stdint.h" // Integer declarations
#include "include/bios32.h" // BIOS32 calls (for switching)

// Typedefs

// vbeInfoBlock_t - A block of information for VESA VBE
typedef struct {
    char *signature; // Block signature, should always be "VESA"
    uint16_t version; // Version of VBE
    uint16_t oemStringPtr[2]; // Pointer to OEM
    uint8_t features[4]; // Available features
    uint32_t *videoModePtr;
    uint16_t totalMemory; // Total video memory (in number of 64KB blocks)
} __attribute__((packed)) vbeInfoBlock_t;

// vbeModeInfo_t - VBE mode information (structure originated from https://wiki.osdev.org/Getting_VBE_Mode_Info)
typedef struct {
    uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__((packed)) vbeModeInfo_t;


// Inline functions



// Functions
void vesaInit();


#endif