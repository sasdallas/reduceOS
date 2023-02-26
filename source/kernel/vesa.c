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
vbeInfoBlock_t *vbeInfo; // 0x2000 is the address for buffer ES:DI to read in

// (VBE mode info does not need to be global)

bool supportsVBE = false; // (set by vesaGetVbeInfo)


// Static functions

// (static) vesaGetVbeInfo() - Sets global variable vbeInfo to VBE info
void vesaGetVbeInfo() {
    // First, create registers for the 0x10 call.
    // ES:DI must contain the buffer to store the info in - we are going to be using vbeInfo address 0x2000



    REGISTERS_16 in = {0};
    

    in.di = 0x7E00;
    in.ax = 0x4F00;

    REGISTERS_16 out = {0};

    
    // Next, call BIOS interrupt 0x10
    bios32_service(0x10, &in, &out);
    
    // Finally, update supportsVBE to the proper value.
    supportsVBE = (out.ax == 0x4F);
    memcpy(vbeInfo, (void*)0x7E00, sizeof(vbeInfoBlock_t));
}

// (static) vesaGetVbeModeInfo(uint16_t mode) - Returns information on a VESA mode.
static vbeModeInfo_t *vesaGetVbeModeInfo(uint16_t mode) {
    // We are doing this a bit different from vesaGetVbeModeInfo - instead storing at a temporary address and returning instead of setting a global value.
    vbeModeInfo_t *modeInfo;

    // Setup registers for bios32 call.
    REGISTERS_16 in = {0};
    in.ax = 0x4F01;
    in.cx = mode;
    in.di = 0x7E00 + 1024; // Address pointer.

    REGISTERS_16 out = {0};

    // Call interrupt 0x10 with in and out registers
    bios32_service(0x10, &in, &out);
    
    // Copy data from temporary address to modeInfo and return.
    memcpy(modeInfo, (void*)0x7E00 + 1024, sizeof(vbeModeInfo_t));

    return modeInfo;
} 


// Functions

// vesaDetectMode(int x, int y, int d) - Check for a mode on the current video card (where X and Y are resolution and d is colordepth) - returns the mode number for it
uint16_t vesaDetectMode(int x, int y, int d) {
    // Get modes and iterate through them to check    
    uint16_t *modes = (uint16_t*)vbeInfo->videoModePtr;
    uint16_t currentMode = *modes++;

    while (currentMode != 0xFFFF) {
        // Get the mode information and compare it against arguments
        vbeModeInfo_t *modeInfo = vesaGetVbeModeInfo(currentMode);
        if (modeInfo->width == x && modeInfo->height == y && modeInfo->bpp == d) {
            return currentMode;
        }

        currentMode = *modes++;
    }

    // We didn't find anything.
    return -1;
}



// vesaInit() - Initialize VESA VBE graphics
void vesaInit() {
    
    
    serialPrintf("video mode pointer is 0x%x (signature is %s)\n", vbeInfo->videoModePtr, *vbeInfo->signature);
    uint16_t *modes = (uint16_t*)vbeInfo->videoModePtr;
    uint16_t currentMode = *modes++;

    for (int i = 0; i < 1; i++) {
        
        vbeModeInfo_t *modeInfo = vesaGetVbeModeInfo(currentMode);
        printf("Found mode: %d - %d x %d with colordepth %d\n", currentMode, modeInfo->width, modeInfo->height, modeInfo->bpp);
        
        currentMode = *modes++;
    }
}