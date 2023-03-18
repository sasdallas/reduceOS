// ==========================================================
// vesa.c - VESA VBE graphics handler
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// VESA VBE is a type of graphics "mode", like VGA
// However, VESA VBE is different from VGA in the fact that it has its own "modes"
// These modes can correspond to different resolutions and colordepths
// For more information, see https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer

#include "include/vesa.h" // Main header file

// Variables

vbeInfoBlock_t vbeInfo;
bool isVBESupported = false;

uint32_t *vbeBuffer; // This is the buffer we will be drawing in
int selectedMode = -1; // The current mode selected.
uint32_t modeWidth, modeHeight, modeBpp = 0; // The height and width of the mode





// Static functions

// (static) vbeGetInfo() - Gets VBE information.
static void vbeGetInfo() {
    // Set up the registers for our INT 0x10 call.
    REGISTERS_16 in, out = {0};
    in.ax = 0x4F00;
    in.di = 0x7E00;

    // Call the BIOS for our interrupt.
    bios32_service(0x10, &in, &out);

    if (out.ax != 0x004F) {
        panic("VBE", "vbeGetModeInfo", "BIOS 0x10 call failed");
    }

    // Copy the data to vbeInfo.
    memcpy(&vbeInfo, (void*)0x7E00, sizeof(vbeInfoBlock_t));

    // Change supported variable.
    isVBESupported = (out.ax == 0x4F);
}

// (static) vbeGetModeInfo(uint16_t mode) - Returns information on a certain mode.
static vbeModeInfo_t vbeGetModeInfo(uint16_t mode) {
    // Like before, setup the registers for our INT 0x10 call.
    // This time, however, change AX to be 0x4F01 to signify that we want mode info.
    vbeModeInfo_t modeInfo;

    REGISTERS_16 in = {0};
    in.ax = 0x4F01;
    in.cx = mode;
    in.di = 0x7E00 + 1024;

    REGISTERS_16 out = {0};

    // Call the BIOS for our interrupt.
    bios32_service(0x10, &in, &out);

    if (out.ax != 0x004F) {
        panic("VBE", "vbeGetModeInfo", "BIOS 0x10 call failed");
    }

    // Copy the data.
    memcpy(&modeInfo, (void*)0x7E00 + 1024, sizeof(vbeModeInfo_t));
    return modeInfo;
}

// DEBUG FUNCTION!!
static void vesaPrintModes() {
    uint16_t *modes = (uint16_t*)vbeInfo.videoModePtr;
    uint16_t currentMode = *modes++;

    vbeModeInfo_t modeInfo;

    // Possible bug here - we are supposed to stop on currentMode being 0xFFFF, but this crashes a lot until it randomly works - unsure if bug with code or QEMU??
    // For now just choose the best resolution we can use without crashing the entire system.
    for (int i = 0; i < 10; i++) {
        modeInfo = vbeGetModeInfo(currentMode);
        serialPrintf("Found mode %d - %d x %d with colordepth %d (mode is 0x%x)\n", currentMode, modeInfo.width, modeInfo.height, modeInfo.bpp, currentMode);
        currentMode = *modes++;
    }
}





// Functions (non-static)

// vbeSetMode(uint32_t mode) - Sets a VBE mode using BIOS32
void vbeSetMode(uint32_t mode) {
    // Like all BIOS32 calls, setup the registers first.
    REGISTERS_16 in = {0};
    in.ax = 0x4F02;
    in.bx = mode;

    REGISTERS_16 out = {0};

    // Call BIOS interrupt 0x10.
    bios32_service(0x10, &in, &out);

    if (out.ax != 0x004F) {
        panic("VBE", "vbeGetModeInfo", "BIOS 0x10 call failed");
    }
}

// vbeGetMode(uint32_t width, uint32_t height, uint32_t color_depth) - Returns the VBE mode with the parameters given.
uint32_t vbeGetMode(uint32_t width, uint32_t height, uint32_t color_depth) {
    // First, get the mode list from the VBE video mode pointer.
    uint16_t *modes = (uint16_t*)vbeInfo.videoModePtr;
    uint16_t currentMode = *modes++;

    // This function has a potential bug, check vesaPrintModes() for the bug description.

    // Locate the mode.
    while (currentMode != 0xFFFF) {
        // Get the mode info and compare it against the parameters.
        vbeModeInfo_t modeInfo = vbeGetModeInfo(currentMode);
        
        if (modeInfo.width == width && modeInfo.height == height && modeInfo.bpp == color_depth) {
            return currentMode; // Found the mode!
        }

        currentMode = *modes++; // Keep searching!
    }

    return -1; // We didn't find the mode :(
}



// Helper function to convert to a VBE RGB value.
uint32_t RGB_VBE(uint8_t r, uint8_t g, uint8_t b) {
	uint32_t color = r;
	color <<= 16;
	color |= (g << 8);
	color |= b;
	return color;
}


// vesaInit() - Initializes VESA VBE.
void vesaInit() {
    // First, get VBE info and check if supported.
    vbeGetInfo();

    if (!isVBESupported) {
        serialPrintf("vesaInit: VBE is not supported.\n");
        return;
    }

    // There is a bug preventing us from checking some video modes - check vesaPrintModes() for the bug description.
    // For now, just use 800x600 with colordepth 32

    uint32_t mode = vbeGetMode(800, 600, 32);
    if (mode == -1) return; // No valid mode was found.

    // Get a bit more information on the mode.
    vbeModeInfo_t modeInfo = vbeGetModeInfo(mode);

    // Change variables to reflect on modeInfo.
    selectedMode = mode;
    modeWidth = modeInfo.width;
    modeHeight = modeInfo.height;
    modeBpp = modeInfo.bpp;
    vbeBuffer = (uint32_t*)modeInfo.framebuffer;

    // Now, switch the mode.
    vbeSetMode(mode);

    int y = 0;
    int r, g, b;
    r = 5;
    g = 0;
    b = 0;

    bool finalSteps = false;

    /*
    for (y = 0; y < modeHeight; y++) {
        for (int x = 0; x < modeWidth; x++) {
            vbePutPixel(x, y, RGB_VBE(r, g, b));
        }
    
        // All this code just for a rainbow wave.
        if (r > 0 && r < 255 && g == 0) {
            r += 5;   
        } else if (r >= 255 && g >= 0 && g < 255) {
            g += 5;
        } else if (r >= 255 && g >= 255) {
            r -= 5;
        } else if (r < 255 && r > 0 && g >= 255) {
            r -= 5;
        } else if (r == 0 && g >= 255 && b == 0) {
            b += 5;
        } else if (g >= 255 && b < 255 && b > 0) {
            b += 5;
        } else if (g >= 255 && b >= 255) {
            g -= 5;
        } else if (g < 255 && g > 0 && b >= 255) {
            g -= 5;
        } else if (r == 0 && g == 0 && b >= 255) {
            r += 5;
        } else if (r > 0 && r < 255 && b >= 255) {
            r += 5;
        } else if (r >= 255 && b >= 255 && g == 0) {
            b -= 5;
        } else if (b < 255 && b > 5 && r >= 255) {
            b -= 5;
        } else if (b == 5 && r >= 255) {
            finalSteps = true; // Some extra code for assistance.
            b = 0; 
            r -= 5;
        } else if (finalSteps && b == 0 && r < 255 && r > 0) {
            r -= 5;
        } else if (finalSteps && r == 0) {
            finalSteps = false;
            r = 5;
        }
    } */

    
}



// Most of the graphics functions in here should not be used, instead using other graphics functions.

// vbePutPixel(int x, int y, uint32_t color) - Puts a pixel on the screen.
void vbePutPixel(int x, int y, uint32_t color) {
    // Get the location of the pixel.
    uint32_t p = y * modeWidth + x;
    *(vbeBuffer + p) = color;
}



// Here we have some functions that I should probably move to another file.
