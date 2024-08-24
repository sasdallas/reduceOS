// ==================================================
// bitmap.c - reduceOS bitmap library
// ==================================================
// Written originally by szhou42, adapted into reduceOS. Credit to him.

#include <kernel/bitmap.h> // Main header file
#include <kernel/ext2.h>



char *addr = NULL;

// bitmap_loadBitmap(fsNode_t *node) - Loads the bitmap into memory and returns the bitmap_t object.
bitmap_t *bitmap_loadBitmap(fsNode_t *node) {
    if (!node) return NULL;

    // Allocate memory for where the bitmap data will go
    
    uint8_t *image_data = kmalloc(node->length + 1024 * 3); // extra 3 KB to be safe
    memset(image_data, 0, node->length + 1024 * 3);
    serialPrintf("image_data allocated to 0x%x\n", image_data);

    // Read in the image data
    int ret = node->read(node, 0, node->length, image_data); 
    if (ret != node->length) {
        serialPrintf("bitmap_loadBitmap: Failed to read in bitmap data, returned %i\n", ret);
        return NULL;
    } 

    // Basically do what createBitmap does but with a different start address
    bitmap_fileHeader_t *h = image_data;

    if (h->type != 0x4D42) {
        serialPrintf("bitmap_loadBitmap: Cannot load bitmap - signature is not 0x4D42 (BM). Signature given: 0x%x\n", h->type);
        kfree(image_data);
        return NULL;
    }

    uint32_t offset = h->offbits;

    // Setup the infoheader
    bitmap_infoHeader_t *info = image_data + sizeof(bitmap_fileHeader_t);

    // Setup the bitmap_t we are returning
    bitmap_t *bmp = kmalloc(sizeof(bitmap_t));
    
    bmp->width = info->width;
    bmp->height = info->height;
    bmp->imageBytes = (void*)((unsigned int)image_data + offset);
    bmp->offset = offset;
    bmp->buffer = image_data;
    bmp->totalSize = h->size;
    bmp->bpp = info->bitcount;
    bmp->bufferSize = node->length;

    addr = image_data;
    return bmp;
}

// OBSOLETE
bitmap_t *createBitmap() {
    panic("bitmap", "createBitmap", "Obsolete function was called.");
}


// displayBitmap(bitmap_t *bmp, int x, int y) - Displays a bitmap image on 2nd framebuffer
void displayBitmap(bitmap_t *bmp, int x, int y) {
    // NOTE: x and y refer to the top left starting point of the bitmap, since I could honestly care less about my future self. 
    // NOTE FROM THE FUTURE: I hate you.

    if (!bmp) return; // Stupid users.

    uint8_t *image = bmp->imageBytes;
    int j = 0;
    
    uint32_t height = 0;
    if (bmp->height > 764) height = 764; // BUG: This isn't even used.
    else height = bmp->height;


    for(int i = 0; i < bmp->height-4-y; i++) {
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
