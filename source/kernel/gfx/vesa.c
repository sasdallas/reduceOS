// ==========================================================
// vesa.c - VESA VBE graphics handler
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// VESA VBE is a type of graphics "mode", like VGA
// However, VESA VBE is different from VGA in the fact that it has its own "modes"
// These modes can correspond to different resolutions and colordepths
// For more information, see https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer

#include <kernel/vesa.h> // Main header file

// Variables

vbeInfoBlock_t vbeInfo;
bool isVBESupported = false;

uint8_t *vbeBuffer; // This is the buffer we will be drawing in
int selectedMode = -1; // The current mode selected.
uint32_t modeWidth, modeHeight, modeBpp, modePitch = 0; // The height and width of the mode

uint32_t *framebuffer; // A seperate framebuffer.


bool VESA_Initialized = false;


// Double buffering is utilized in this VBE driver. Basically, that means that instead of drawing directly to video memory, you draw to a framebuffer (the one above).
// For now we use manual swapping, but potentially later PIT will swap each tick.


// Static functions

// (static) vbeGetInfo() - Gets VBE information.
static void vbeGetInfo() {
    // Set up the registers for our INT 0x10 call.
    REGISTERS_16 in, out = {0};
    in.ax = 0x4F00;
    in.di = 0x7E00;

    // Call the BIOS for our interrupt.
    bios32_call(0x10, &in, &out);

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
    bios32_call(0x10, &in, &out);

    if (out.ax != 0x004F) {
        panic("VBE", "vbeGetModeInfo", "BIOS 0x10 call failed");
    }

    // Copy the data.
    memcpy(&modeInfo, (void*)0x7E00 + 1024, sizeof(vbeModeInfo_t));
    return modeInfo;
}

// DEBUG FUNCTION!!
void vesaPrintModes() {
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
    bios32_call(0x10, &in, &out);

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

uint32_t VGA_TO_VBE(uint8_t vgaColor) {
    switch (vgaColor) {
        case COLOR_BLACK:
            return RGB_VBE(0, 0, 0);
        case COLOR_WHITE:
            return RGB_VBE(255, 255, 255);
        case COLOR_BLUE:
            return RGB_VBE(0, 0, 170);
        case COLOR_GREEN:
            return RGB_VBE(0, 170, 0);
        case COLOR_CYAN:
            return RGB_VBE(0, 170, 170);
        case COLOR_RED:
            return RGB_VBE(170, 0, 0);
        case COLOR_MAGENTA:
            return RGB_VBE(170, 0, 170);
        case COLOR_BROWN:
            return RGB_VBE(170, 85, 0);
        case COLOR_LIGHT_GRAY:
            return RGB_VBE(170, 170, 170); // why did we do gray as light gray
        case COLOR_DARK_GRAY:
            return RGB_VBE(85, 85, 85);
        case COLOR_LIGHT_BLUE:
            return RGB_VBE(85, 85, 255);
        case COLOR_LIGHT_GREEN:
            return RGB_VBE(85, 85, 255);
        case COLOR_LIGHT_CYAN:
            return RGB_VBE(85, 255, 255);
        case COLOR_LIGHT_RED:
            return RGB_VBE(255, 85, 85);
        case COLOR_LIGHT_MAGENTA:
            return RGB_VBE(255, 85, 255);
        case COLOR_YELLOW:
            return RGB_VBE(255, 255, 85);
        default:
            serialPrintf("VGA_TO_VBE: stupid users (color = %i)\n", vgaColor);
            return;
    }
    // lmao
}



// vesa_initGOP(multiboot_info *info) - Initialize VESA using a GOP framebuffer
int vesa_initGOP(multiboot_info *info) {
    if (VESA_Initialized) return -2; // Already done for us.
    if (!info->framebuffer_addr) {
        return -1; // Nope
    }

    // Identity map framebuffer!
    //serialPrintf("Framebuffer address: %p\n", (uintptr_t)info->framebuffer_addr);
    pmm_deinitRegion((uintptr_t)info->framebuffer_addr, 1024*768);
    vmm_allocateRegion((uintptr_t)info->framebuffer_addr, (uintptr_t)info->framebuffer_addr, 1024*768*4);

    vbeBuffer = (void*)info->framebuffer_addr;
    modeWidth = info->framebuffer_width;
    modeHeight = info->framebuffer_height;
    modeBpp = info->framebuffer_bpp;
    modePitch = info->framebuffer_pitch;

    

    framebuffer = kmalloc(1024*768*4);

    VESA_Initialized = true;

    return 0;
}


// vesaInit() - Initializes VESA VBE.
void vesaInit() {
    if (VESA_Initialized) return; // Already done for us

    // First, get VBE info and check if supported.
    vbeGetInfo();

    if (!isVBESupported) {
        serialPrintf("vesaInit: VBE is not supported.\n");
        return;
    }

    // There is a bug preventing us from checking some video modes - check vesaPrintModes() for the bug description.
    // For now, just use 1024x768 with colordepth 32

    // uint32_t mode = vbeGetMode(800, 600, 32);
    // if (mode == -1) return; // No valid mode was found.
    

    // Bypass getting the mode and just set it like so.
    uint32_t mode = 0x144 | 0x4000;
    
    // Get a bit more information on the mode.
    vbeModeInfo_t modeInfo = vbeGetModeInfo(mode);

    // Identity map framebuffer!
    vmm_allocateRegion(modeInfo.framebuffer, modeInfo.framebuffer, 1024*768*4);

    // Make sure PMM knows not to allocate here
    pmm_deinitRegion(modeInfo.framebuffer, 1024*768);

    // Change variables to reflect on modeInfo.
    selectedMode = mode;
    modeWidth = modeInfo.width;
    modeHeight = modeInfo.height;
    modeBpp = modeInfo.bpp;
    modePitch = modeInfo.pitch;
    vbeBuffer = (uint8_t*)modeInfo.framebuffer;
    
    // Now, switch the mode.
    vbeSetMode(mode);

    framebuffer = kmalloc(1024*768*4); // BUG: Why *4?

    serialPrintf("vesaInit: Allocated framebuffer to 0x%x - 0x%x\n", framebuffer, (int)framebuffer + (1024*768*4));
    serialPrintf("vesaInit: vbeBuffer is from 0x%x - 0x%x\n", vbeBuffer, (int)vbeBuffer + (1024*768*4));

    // Because framebuffer is big and stupid pmm will get confused and try to still allocate memory INSIDE of it
    // So we need to fix that by telling PMM to go pound sand and deinitialize the region
    pmm_deinitRegion(framebuffer, 1024*768*4);

    // This will stop other functions from trying to initialize VESA.
    VESA_Initialized = true;
}



// vbeSwitchBuffers() - Switches the framebuffers
int vbeSwitchBuffers() {
    if (!isVBESupported) return -1; // Not supported - TODO: Move me into VESA_Initialized.
    if (!VESA_Initialized) return -1; // Framebuffer not initialized.
    
    // Switch the framebuffers
    for (int i = 0; i < 1024*768; i++) {
        ((uint32_t*)vbeBuffer)[i] = framebuffer[i]; // Is this is a bug? I don't like the typecast.
    }

    return 0; // Updated buffers successfully.
}


// Most of the graphics functions in here should not be used, instead using other graphics functions.

// vbePutPixel(int x, int y, uint32_t color) - Puts a pixel on the screen.
void vbePutPixel(int x, int y, uint32_t color) {
    // Get the location of the pixel.
    uint32_t p = y * (modePitch/4) + x;
    *(framebuffer + p) = color;
}

uint32_t vbeGetPixel(int x, int y) {
    return *(framebuffer + (y * (modePitch/4) + x));
}

