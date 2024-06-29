// ==================================================
// bitmap.c - reduceOS bitmap library
// ==================================================
// Written originally by szhou42, adapted into reduceOS. Credit to him.

#include "include/bitmap.h" // Main header file

extern char _binary_source_images_cheeseburger_bmp_start;
extern char _binary_source_images_cheeseburger_bmp_end;


bitmap_t *createBitmap() {
    // The image we attempt to display is burned into the code by the linker (thankfully, initrd's interface sucks and that's totally not my fault)
    bitmap_t *ret = kmalloc(sizeof(bitmap_t));
    char *start_addr = &_binary_source_images_cheeseburger_bmp_start;
    bitmap_fileHeader_t *h = start_addr;

    // Validate signature
    if (h->type != 0x4D42) {
        serialPrintf("createBitmap: WARNING! Signature is not 0x4D42 (BM)! Signature is: 0x%x\n", h->type);
    } else {
        serialPrintf("createBitmap: Signature OK on bitmap\n");
    }


    uint32_t offset = h->offbits;
    serialPrintf("createBitmap: Bitmap offset = %u\n", offset);
    serialPrintf("createBitmap: Bitmap size = %u\n", h->size);

    // Setup infoheader
    bitmap_infoHeader_t *info = start_addr + sizeof(bitmap_fileHeader_t);

    ret->width = info->width;
    ret->height = info->height;
    ret->imageBytes = (void*)((unsigned int)start_addr + offset);
    ret->buffer = start_addr;
    ret->totalSize = h->size;
    ret->bpp = info->bitcount;

    serialPrintf("createBitmap: Bitmap dimensions are %u x %u\n", ret->width, ret->height);
    serialPrintf("createBitmap: Image is located at 0x%x\n", ret->imageBytes);
    serialPrintf("createBitmap: Successfully loaded bitmap.\n");
    return ret;
}

// displayBitmap(bitmap_t *bmp, int x, int y) - Displays a bitmap image on 2nd framebuffer
void displayBitmap(bitmap_t *bmp, int x, int y) {
    // NOTE: x and y refer to the top left starting point of the bitmap, since I could honestly care less about my future self. 
    if (!bmp) return; // Stupid users.

    uint8_t *image = bmp->imageBytes;
    int j = 0;
    
    uint32_t height = 0;
    if (bmp->height > 764) height = 764; // BUG BUG BUG BUG BUG
    else height = bmp->height;

    for(int i = 0; i < bmp->height-4; i++) {
        // Copy the image to the framebuffer.
        char * imageRow = image + i * bmp->width * 3; // Needed to get BGR values.
        uint32_t * framebufferRow = (void*)framebuffer + (bmp->height - 1 - i) * bmp->width * 4;
        j = 0;
        
        for(int k = 0; k < bmp->width; k++) {
            uint32_t b = imageRow[j++] & 0xff; // BGR values are encoded in the bitmap
            uint32_t g = imageRow[j++] & 0xff;
            uint32_t r = imageRow[j++] & 0xff;
            uint32_t rgb = ((r << 16) | (g << 8) | (b)) & 0x00ffffff;
            rgb = rgb | 0xff000000;
            framebufferRow[k] = rgb;
        }
    }
    
}