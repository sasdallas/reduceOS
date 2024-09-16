// ==========================================================
// vesa.c - VESA VBE graphics handler
// ==========================================================
// This file is a part of the reduceOS C kernel. Please credit me if you use this code.

// VESA VBE is a type of graphics "mode", like VGA
// However, VESA VBE is different from VGA in the fact that it has its own "modes"
// These modes can correspond to different resolutions and colordepths
// For more information, see https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer

#include <kernel/vesa.h> // Main header file
#include <kernel/vfs.h>
#include <kernel/fb.h>
#include <kernel/syscall.h>
#include <libk_reduced/errno.h>
#include <libk_reduced/stdio.h>


// Variables

vbeInfoBlock_t vbeInfo;
bool isVBESupported = false;

uint8_t *vbeBuffer; // This is the buffer we will be drawing in
int selectedMode = -1; // The current mode selected.
uint32_t modeWidth, modeHeight, modeBpp, modePitch = 0; // The height and width of the mode

uint8_t *framebuffer; // A seperate framebuffer.


bool VESA_Initialized = false;


// Double buffering is utilized in this VBE driver. Basically, that means that instead of drawing directly to video memory, you draw to a framebuffer (the one above).
// For now we use manual swapping, but potentially later PIT will swap each tick.


// Caller functions

// vbeGetInfo() - Gets VBE information.
void vbeGetInfo() {
    // Set up the registers for our INT 0x10 call.
    REGISTERS_16 in, out = {0};
    in.ax = 0x4F00;
    in.di = 0x7E00;

    // Call the BIOS for our interrupt.
    bios32_call(0x10, &in, &out);



    if (out.ax != 0x004F) {
        return;
    }

    // Copy the data to vbeInfo.
    memcpy(&vbeInfo, (void*)0x7E00, sizeof(vbeInfoBlock_t));

    // Change supported variable.
    isVBESupported = (out.ax == 0x4F);
}


// vbeGetModeInfo(uint16_t mode) - Returns information on a certain mode.
int vbeGetModeInfo(uint16_t mode, vbeModeInfo_t *modeInfo) {
    // Like before, setup the registers for our INT 0x10 call.
    // This time, however, change AX to be 0x4F01 to signify that we want mode info.

    REGISTERS_16 in = {0};
    in.ax = 0x4F01;
    in.cx = mode;
    in.di = 0x7E00 + 1024;

    REGISTERS_16 out = {0};

    // Call the BIOS for our interrupt.
    bios32_call(0x10, &in, &out);

    if (out.ax != 0x004F) {
        return -1;
    }

    // Copy the data.
    memcpy(modeInfo, (void*)0x7E00 + 1024, sizeof(vbeModeInfo_t));
    return 0;
}

// DEBUG FUNCTION!!
void vesaPrintModes(bool showModesToConsole) {
    vbeGetInfo();
    uint16_t *modes = (uint16_t*)vbeInfo.videoModePtr;
    uint16_t currentMode = *modes++;

    vbeModeInfo_t *modeInfo = kmalloc(sizeof(vbeModeInfo_t));

    // Possible bug here - we are supposed to stop on currentMode being 0xFFFF, but this crashes a lot until it randomly works - unsure if bug with code or QEMU??
    // For now just choose the best resolution we can use without crashing the entire system.
    for (int i = 0; i < 10; i++) {
        int ret = vbeGetModeInfo(currentMode, modeInfo);
        if (ret == 0) serialPrintf("Found mode %d - %d x %d with colordepth %d (mode is 0x%x)\n", currentMode, modeInfo->width, modeInfo->height, modeInfo->bpp, currentMode);
        if (ret == 0 && showModesToConsole) printf("Found mode %d - %d x %d with colordepth %d (mode is 0x%x)\n", currentMode, modeInfo->width, modeInfo->height, modeInfo->bpp, currentMode);
        currentMode = *modes++;
    }

    kfree(modeInfo);
}




// Functions (non-static)

// vbeSetMode(uint32_t mode) - Sets a VBE mode using BIOS32
int vbeSetMode(uint32_t mode) {
    // Like all BIOS32 calls, setup the registers first.
    REGISTERS_16 in = {0};
    in.ax = 0x4F02;
    in.bx = mode;

    REGISTERS_16 out = {0};

    // Call BIOS interrupt 0x10.
    bios32_call(0x10, &in, &out);

    if (out.ax != 0x004F) {
        return -1;
    }

    

    return 0;
}

// vbeGetMode(uint32_t width, uint32_t height, uint32_t color_depth) - Returns the VBE mode with the parameters given.
uint32_t vbeGetMode(uint32_t width, uint32_t height, uint32_t color_depth) {
    // It is possible (and likely) that we are using GRUB's framebuffer and vbeInfo hasn't been setup
    vbeGetInfo();

    // First, get the mode list from the VBE video mode pointer.
    uint16_t *modes = (uint16_t*)vbeInfo.videoModePtr;
    uint16_t currentMode = *modes++;

    // Locate the mode.
    while (currentMode != 0xFFFF) {
        // Get the mode info and compare it against the parameters.
        vbeModeInfo_t modeInfo;
        vbeGetModeInfo(currentMode, &modeInfo); // Unsafe(?)
        
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
            return RGB_VBE(0, 0, 0);
    }
    // lmao
}


// vesaDoGOP() 


// vesaInit() - Initializes VESA VBE.
int vesaInit() {
    
    if (VESA_Initialized) return -EALREADY; // Already done for us

    // First, get VBE info and check if supported.
    vbeGetInfo();

    if (!isVBESupported) {
        serialPrintf("vesaInit: VBE is not supported.\n");
        return -ENOTSUP;
    }

    // Search for 1024x768x32 and 800x600x32
    uint32_t mode = vbeGetMode(1024, 768, 32);
    if ((int)mode == -1) {
        mode = vbeGetMode(800, 600, 32);
        if ((int)mode == -1) {
            // No modes possible that we could find. Shame!
            return -ENOENT;
        }
    }
    
    // The rest of this is actually bug territory

    // Get a bit more information on the mode.
    vbeModeInfo_t *modeInfo = kmalloc(sizeof(vbeModeInfo_t));
    vbeGetModeInfo(mode, modeInfo);

    if ((int)modeInfo == NULL) {
        panic_prepare();
        printf("*** The call to VESA VBE hardware to get information on mode 0x%x failed.\n", mode);
        printf("*** Unknown cause. (this is likely BIOS32 at fault)\n");
        printf("\nThis is likely a bug with the driver. Please open an issue on GitHub!\n");
        asm volatile ("hlt");
        for (;;);
    }

    // Identity map framebuffer!
    vmm_allocateRegion(modeInfo->framebuffer, modeInfo->framebuffer, modeInfo->width*modeInfo->height*4);

    // Make sure PMM knows not to allocate here
    pmm_deinitRegion(modeInfo->framebuffer, modeWidth*modeHeight);

    // Change variables to reflect on modeInfo.
    selectedMode = mode;
    modeWidth = modeInfo->width;
    modeHeight = modeInfo->height;
    modeBpp = modeInfo->bpp;
    modePitch = modeInfo->pitch;
    vbeBuffer = (uint8_t*)modeInfo->framebuffer;
    
    // Now, switch the mode.
    if (vbeSetMode(mode) == -1) {
        // That didn't work...
        mode = vbeGetMode(800, 600, 32); // Try to grab it for 800x600x32
        if ((int)mode == -1) {
            panic_prepare();
            printf("*** No suitable video mode could be found.\n");
            printf("*** Fallback options not available.\n");
            asm volatile ("hlt");
            for (;;);
            __builtin_unreachable();
        }
        
        if (vbeSetMode(mode) == -1) {
            panic_prepare();
            printf("*** No suitable video mode could be set (tried 800x600x32 and 1024x768x32).\n");
            printf("*** Fallback options not available.\n");
            asm volatile ("hlt");
            for (;;);
            __builtin_unreachable();
        }
        
    }


    framebuffer = kmalloc(modeWidth*modeHeight*4); // The *4 is for the size of a uint32_t. 

    serialPrintf("vesaInit: Allocated framebuffer to 0x%x - 0x%x\n", framebuffer, (int)framebuffer + (modeWidth*modeHeight*4));
    serialPrintf("vesaInit: vbeBuffer is from 0x%x - 0x%x\n", vbeBuffer, (int)vbeBuffer + (modeWidth*modeHeight*4));

    // Because framebuffer is big and stupid pmm will get confused and try to still allocate memory INSIDE of it
    // So we need to fix that by telling PMM to go pound sand and deinitialize the region
    // TODO: Believe this was fixed.
    pmm_deinitRegion((uintptr_t)framebuffer, modeWidth*modeHeight*4);

    // This will stop other functions from trying to initialize VESA.
    VESA_Initialized = true;

    return 0;
}



// vbeSwitchBuffers() - Switches the framebuffers
int vbeSwitchBuffers() {
    if (!VESA_Initialized) return -1; // Driver not initialized
    
    // Switch the framebuffers
    // I don't know why, but this just feels faster?
    for (uint32_t i = 0; i < modeWidth * modeHeight; i++) {
        ((uint32_t*)vbeBuffer)[i] = ((uint32_t*)framebuffer)[i];
    }

    return 0; // Updated buffers successfully.
}


// Most of the graphics functions in here should not be used, instead using other graphics functions.

// vbePutPixel(int x, int y, uint32_t color) - Puts a pixel on the screen.
void vbePutPixel(int x, int y, uint32_t color) {
    // Get the location of the pixel.
    uint32_t p = x * 4 + y*modePitch;
    framebuffer[p] = color & 255;
    framebuffer[p + 1] = (color >> 8) & 255;
    framebuffer[p + 2] = (color >> 16) & 255;

}

uint32_t vbeGetPixel(int x, int y) {
    return *(framebuffer + (y * (modePitch/4) + x));
}



/* /device/fbX FUNCTIONS */

// I/O control function
int vesa_ioctl(fsNode_t *node, unsigned long request, void *argp) {
    if (!VESA_Initialized) return -EBUSY; // -16 can also mean device not initialized

    fb_info_t *info;
    
    switch (request) {
        case FBIOGET_SCREENH:
            syscall_validatePointer(argp, "VESAIOCTL");
            *((size_t*)argp) = modeHeight;
            return 0;
        case FBIOGET_SCREENW:
            syscall_validatePointer(argp, "VESAIOCTL");
            *((size_t*)argp) = modeWidth;
            return 0;
        case FBIOGET_SCREENDEPTH:
            syscall_validatePointer(argp, "VESAIOCTL");
            *((size_t*)argp) = modeBpp;
            return 0;
        case FBIOGET_SCREENPITCH:
            syscall_validatePointer(argp, "VESAIOCTL");
            *((size_t*)argp) = modePitch;
            return 0;
        case FBIOGET_SCREENADDR:
            syscall_validatePointer(argp, "VESAIOCTL");
            uintptr_t mapOffset;
            if (*(uintptr_t*)argp != 0) { 
                // The user wants us to map to a specific pointer. 
                syscall_validatePointer((void*)(*(uintptr_t*)argp), "VESAIOCTL");
                mapOffset = *(uintptr_t*)argp;
            } else {
                mapOffset = (uintptr_t)kmalloc(modeWidth * modeHeight * 4);
            }

            for (uint32_t i = mapOffset; i < mapOffset + (modeWidth*modeHeight*4); i += 0x1000) {
                vmm_allocateRegionFlags((uintptr_t)vmm_getPhysicalAddress(vmm_getCurrentDirectory(), (uintptr_t)(vbeBuffer + (i-mapOffset))), i, 4096, 1, 1, 1);
            }

            *((uintptr_t*)argp) = mapOffset;
            return 0;
        case FBIOPUT_SCREENINFO:
            // They want to put screen info, or set a specific method. Therefore, let's follow through.
            syscall_validatePointer(argp, "VESAIOCTL");
            if (!argp) return -EINVAL;

            info = (fb_info_t*)argp;
            uint32_t mode = vbeGetMode(info->width, info->height, info->bpp);
            if ((int)mode == -1) return -EINVAL;
            vbeSetMode(mode);
            return 0;
        case FBIOPUT_SCREENADDR:
            panic("VESA", "ioctl", "Kernel trap on unimplemented function FBIOPUT_SCREENADDR.");
            __builtin_unreachable();
        default:
            serialPrintf("vesa_ioctl: Unknown I/O control request 0x%x\n", request);
            break;
    }

    return -EINVAL;
}

int vesa_createVideoDevice(char *devname) {
    if (!devname) return -EINVAL;
    fsNode_t *fnode = kmalloc(sizeof(fsNode_t));
    memset((void*)fnode, 0, sizeof(fsNode_t));
    snprintf(fnode->name, 100, devname);


    fnode->length = 0;
    fnode->flags = VFS_BLOCKDEVICE;
    fnode->mask = 0660;
    fnode->ioctl = vesa_ioctl;

    char *mntpath = kmalloc(strlen("/device/") + strlen(devname) + 1);
    sprintf(mntpath, "/device/%s", devname);
    vfsMount(mntpath, fnode);
    return 0;
}