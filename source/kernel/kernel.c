// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file


// heap.c defined variables.
extern uint32_t placement_address;

// ide_ata.c defined variables
extern ideDevice_t ideDevices[4];

// initrd.c defined variables
extern fsNode_t *initrdDev;

multiboot_info *globalInfo;

extern void switchToUserMode(); // User mode switch function




int getSystemInformation(int argc, char *args[]) {
    uint32_t vendor[32];

    int iedx = 0;
    __cpuid(0x80000002, (uint32_t *)vendor+0x0, (uint32_t *)vendor+0x1, (uint32_t *)vendor+0x2, (uint32_t *)vendor+0x3);
    __cpuid(0x80000003, (uint32_t *)vendor+0x4, (uint32_t *)vendor+0x5, (uint32_t *)vendor+0x6, (uint32_t *)vendor+0x7);
    __cpuid(0x80000004, (uint32_t *)vendor+0x8, (uint32_t *)vendor+0x9, (uint32_t *)vendor+0xa, (uint32_t *)vendor+0xb);

    // Now get if the CPU is 64 or 32.
    // We are going to be calling cpuid (manually, through inline assembly), but first we need to move EAX to 0x80000001 to signify we want to check the capabilities.
    asm volatile("movl $0x80000001, %%eax\n"
                "cpuid\n" : "=d"(iedx) :: "eax", "ebx", "ecx");


    // Tell the user.
    printf("CPU Vendor: %s\n", vendor);
    printf("64 bit capable: %s\n", (iedx & (1 << 29)) ? "YES" : "NO (32-bit)");
    printf("CPU frequency: %u Hz\n", getCPUFrequency());

    printf("Available physical memory: %i KB\n", globalInfo->m_memoryHi - globalInfo->m_memoryLo);
    

    return 1; // Return
}



int dump(int argc, char *args[]) {
    if (argc == 1) { printf("No arguments! Possible arguments: sysinfo, memory <addr>, memoryrange <addr1> <addr2>\n"); return -1; }
    if (!strcmp(args[1], "sysinfo")) {
        // Getting vendor ID and if the CPU is 16, 32, or 64 bits (32 at minimum so idk why we check for 16, but just in case)
        uint32_t vendor[32];
    
        memset(vendor, 0, sizeof(vendor));
    
        __cpuid(0x80000002, (uint32_t *)vendor+0x0, (uint32_t *)vendor+0x1, (uint32_t *)vendor+0x2, (uint32_t *)vendor+0x3);
        __cpuid(0x80000003, (uint32_t *)vendor+0x4, (uint32_t *)vendor+0x5, (uint32_t *)vendor+0x6, (uint32_t *)vendor+0x7);
        __cpuid(0x80000004, (uint32_t *)vendor+0x8, (uint32_t *)vendor+0x9, (uint32_t *)vendor+0xa, (uint32_t *)vendor+0xb);
        
        printf("CPU model: %s (frequency: %i Hz)\n", vendor, getCPUFrequency());
        printf("Available memory: %i KB\n", globalInfo->m_memoryHi - globalInfo->m_memoryLo);
        printf("Drives available:\n");
        printIDESummary();
        printf("PCI devices:\n");
        printPCIInfo();

    } else if (!strcmp(args[1], "memory") && argc > 2) {
        printf("Warning: Dumping memory in the wrong spots can crash the OS.\n");
        int addr = (int)strtol(args[2], NULL, 16);
        int *value;
        memcpy(value, addr, sizeof(void*));
        printf("Value at memory address 0x%x: 0x%x (%i)\n", addr, (uint8_t)value, value);
    } else if (!strcmp(args[1], "memoryrange") && argc > 3) {
        printf("Warning: Dumping memory in the wrong spots can crash the OS.\n");
        int addr1 = (int)strtol(args[2], NULL, 16);
        int addr2 = (int)strtol(args[3], NULL, 16);
        printf("Values from memory addresses 0x%x - 0x%x:\n", addr1, addr2);
        int *value;
        for (addr1; addr1 < addr2; addr1++) {
            memcpy(value, addr1, sizeof(void*));
            printf("0x%x=0x%x", addr1, value);
        }
    } else {
        printf("Invalid arguments, please check if your syntax is correct.\n");
        printf("Possible arguments: sysinfo, memory <addr>, memoryrange <addr1> <addr2>");
        return -1;
    }

    printf("Dump complete\n");
    return 0;
}

int echo(int argc, char *args[]) {
    if (argc > 1) {
        for (int i = 1; i < argc; i++) { printf("%s ", args[i]); }
        printf("\n");
    }

    return 1;
}



int crash(int argc, char *args[]) {
    setKBHandler(false); // Disable keyboard handler.
    printf("Why do you want to crash reduceOS?\n");
    sleep(3000);
    printf("FOR SCIENCE, OF COURSE!");
    sleep(3000);
    panic("kernel", "kmain()", "Error in function crash()");
}

int pciInfo(int argc, char *args[]) { printPCIInfo(); return 1; }



int shutdown(int argc, char *args[]) {
    printf("Shutting down reduceOS...\n");
    asm volatile ("cli");
    asm volatile ("hlt");
}




int getInitrdFiles(int argc, char *args[]) {
    int i = 0;
    struct dirent *node = 0;
    while ((node = readDirectoryFilesystem(fs_root, i)) != 0)
    {
        printf("Found file %s", node->name);
        fsNode_t *fsNode = findDirectoryFilesystem(fs_root, node->name);

        if ((fsNode->flags & 0x7 ) == VFS_DIRECTORY)
        {
            printf(" (directory)\n");
        }
        else
        {
            printf("\n\t contents: ");
            char buf[256];
            uint32_t sz = readFilesystem(fsNode, 0, 256, buf);
            for (int j = 0; j < sz; j++)
                printf("%c", buf[j]);
            
            printf("\n");
        }
        i++;
    }
}


int ataPoll(int argc, char *args[]) {
    printIDESummary();
    return 1;
}

// Used for debugging
int panicTest(int argc, char *args[]) {
    panic("kernel", "panicTest()", "Testing panic function");
}

int readSectorTest(int argc, char *args[]) {
    // BUGGED: Cannot read/write
    // Read a sector from a disk using LBA (after checking if a disk exists with proper capacity).
    int drive = -1;
    for (int i = 0; i < 4; i++) { 
        if (ideDevices[i].reserved == 1 && ideDevices[i].size > 1) {
            printf("Found IDE device with %i KB\n", ideDevices[i].size);
            drive = i;
            break;
        }
    }

    if (drive == -1) {
        printf("No drives found or capacity too low to read sector.\n");
        return -1;
    }

    // write message to drive
    const uint32_t LBA = 0;
    const uint8_t NO_OF_SECTORS = 1;
    char *wbuf = "Hello!";

    // write message to drive
    ideWriteSectors((int)drive, NO_OF_SECTORS, LBA, (uint32_t)&wbuf);
    printf("Wrote data.\n");
    

    char buf[512];
    // read message from drive
    memset(buf, 0, sizeof(buf));
    ideReadSectors((int)drive, NO_OF_SECTORS, LBA, (uint32_t)buf);
    printf("Read data: %s\n", buf);
    return 0;
}





int memoryInfo(int argc, char *args[]) {
    printf("Available physical memory: %i KB\n", globalInfo->m_memoryHi - globalInfo->m_memoryLo);
    return 0;
}



void usermodeMain() {

}

// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(multiboot_info* mem) {
    // Update global multiboot info.
    globalInfo = mem;

    // Get kernel size
    extern uint32_t start;
    uint32_t kernelStart = &start;
    extern uint32_t end;
    uint32_t kernelEnd = &end;
    
    // Some extra stuff
    extern uint32_t text_start;
    extern uint32_t text_end;
    extern uint32_t data_start;
    extern uint32_t data_end;
    extern uint32_t bss_start;
    extern uint32_t bss_end;

    extern uint32_t modeWidth;
    extern uint32_t modeHeight;
    extern uint32_t modeBpp;
    extern uint32_t *vbeBuffer;

    // Perform some basic setup
    initTerminal(); // Initialize the terminal and clear the screen
    updateTerminalColor(vgaColorEntry(COLOR_BLACK, COLOR_LIGHT_GRAY)); // Update terminal color


    // First, setup the top bar.
    printf("reduceOS v1.0 - Development Build");
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 - Development Build")); i++) printf(" ");

    // Next, update terminal color to the proper color.
    updateTerminalColor(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    printf("reduceOS is loading, please wait...\n");

    // Change the bottom text of the terminal (updateBottomText)
    updateBottomText("Loading reduceOS...");

    // Initialize serial logging
    serialInit();
    serialPrintf("reduceOS v1.0-dev - written by sasdallas\n");
    serialPrintf("Kernel location: 0x%x - 0x%x\nText section: 0x%x - 0x%x; Data section: 0x%x - 0x%x; BSS section: 0x%x - 0x%x\n", &kernelStart, &kernelEnd, &text_start, &text_end, &data_start, &data_end, &bss_start, &bss_end);
    serialPrintf("============================================================================================================================\n");
    serialPrintf("Serial logging initialized!\n");
    
    printf("Serial logging initialized on COM1.\n");



    updateBottomText("Initializing HAL...");
    cpuInit();
    printf("HAL initialization completed.\n");


    bios32_init();
    serialPrintf("bios32 initialized successfully!\n");
    
    // Getting vendor ID and if the CPU is 16, 32, or 64 bits (32 at minimum so idk why we check for 16, but just in case)
    uint32_t vendor[32];
    int iedx = 0;
    
    memset(vendor, 0, sizeof(vendor));
    
    __cpuid(0x80000002, (uint32_t *)vendor+0x0, (uint32_t *)vendor+0x1, (uint32_t *)vendor+0x2, (uint32_t *)vendor+0x3);
    __cpuid(0x80000003, (uint32_t *)vendor+0x4, (uint32_t *)vendor+0x5, (uint32_t *)vendor+0x6, (uint32_t *)vendor+0x7);
    __cpuid(0x80000004, (uint32_t *)vendor+0x8, (uint32_t *)vendor+0x9, (uint32_t *)vendor+0xa, (uint32_t *)vendor+0xb);

    // Now get if the CPU is 64 or 32.
    // We are going to be calling cpuid (manually, through inline assembly), but first we need to move EAX to 0x80000001 to signify we want to check the capabilities.
    asm volatile ("movl $0x80000001, %%eax\n"
                "cpuid\n" : "=d"(iedx) :: "eax", "ebx", "ecx");

    
    // Tell the user.
    printf("CPU Vendor: %s\n", vendor);
    printf("64 bit capable: %s\n", (iedx & (1 << 29)) ? "YES" : "NO (32-bit)");
    serialPrintf("User is running with CPU vendor %s (64-bit capable: %s)\n", vendor, (iedx & (1 << 29)) ? "YES" : "NO");





    
    // Initialize Keyboard 
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    setKBHandler(true);
    serialPrintf("Keyboard handler initialized.\n");
    

    // PIT timer
    updateBottomText("Initializing PIT...");
    i86_pitInit();
    serialPrintf("PIT started at 1000hz\n");

    // bios32 stops to work after we initialize paging, so get all calls done now!
    // Initializing VESA will NOT work after this, so we have to ask the user what they want to do now:
    // Update: Found the reason, since paging includes 0x7E00 (temp memory for all bios32 calls), crashing all of reduceOS.

    // Probably a bad hotfix, but it works, I suppose.
    printf("To enter the backup command line, please press 'c' (5 seconds).");
    
    bool didInitVesa = true;
    int timeRemaining = 5000;


    while (true) {
        char c = isKeyPressed();
        if (c == 'c') { 
            didInitVesa = false;
            break;
        } else if (timeRemaining > 0) {
            sleep(1);
            timeRemaining--;
            if (timeRemaining % 1000) printf("\rTo enter the backup command line, please press 'c' (%i seconds)", timeRemaining / 1000);
        } else if (timeRemaining == 0) {
            vesaInit();
            bitmapFontInit();
            break;
        }
    }


   
    printf("\nKernel is loaded at 0x%x, ending at 0x%x\n", kernelStart, kernelEnd);

    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("initPCI: PCI probe completed\n");

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE
    
    // Initialize ACPI
    updateBottomText("Initializing ACPI...");
    acpiInit();
   
    // Initialize paging
    
    // Calculate the initial ramdisk location in memory (panic if it's not there)
    ASSERT(mem->m_modsCount > 0, "kmain", "Initial ramdisk not found (modsCount is 0)");
    uint32_t initrdLocation = *((uint32_t*)mem->m_modsAddr);
    uint32_t initrdEnd = *(uint32_t*)(mem->m_modsAddr + 4);
    placement_address = initrdEnd;
    serialPrintf("GRUB did pass an initial ramdisk.\n");
    
    fs_root = initrdInit(initrdLocation);    
    printf("Initrd image initialized!\n");
    serialPrintf("initrdInit: Initial ramdisk loaded - location is 0x%x and end address is 0x%x\n", initrdLocation, initrdEnd);

    fatInit();
    

    if (didInitVesa) {
        // bitmap_t *wallpaper = createBitmap();
        // displayBitmap(wallpaper);
        // vbeSwitchBuffers();
        
        // Initialize PSF
        psfInit();
        
        
        // bitmapFontDrawString("This is a test.", 50, 50, RGB_VBE(0, 255, 0));
        // gfxDrawRect(80, 80, 100, 100, RGB_VBE(255, 0, 0), false);
        // gfxDrawRect(150, 80, 180, 100, RGB_VBE(255, 0, 0), true);
        // gfxDrawLine(500, 500, 750, 750, RGB_VBE(255, 0, 0));
        // gfxDrawLine(500, 500, 250, 750, RGB_VBE(255, 0, 0));
        // gfxDrawLine(250, 750, 750, 750, RGB_VBE(255, 0, 0));
        vbeSwitchBuffers();
    }


    uint8_t seconds, minutes, hours, days, months;
    int years;

    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &years);
    serialPrintf("rtc_getDateTime: Got date and time from RTC (formatted as M/D/Y H:M:S): %i/%i/%i %i:%i:%i\n", months, days, years, hours, minutes, seconds);

    

    // Skip usermode for now, we'll come back to it after we fix it.

    // Start paging if VBE was not initialized.
    useCommands();
}



void useCommands() {
    clearScreen(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    clearBuffer();

    // The user entered the command handler. We will not return.

    initCommandHandler();
    registerCommand("system", (command*)getSystemInformation);
    registerCommand("echo", (command*)echo);
    registerCommand("crash", (command*)crash);
    registerCommand("pci", (command*)pciInfo);
    registerCommand("initrd", (command*)getInitrdFiles);
    registerCommand("ata", (command*)ataPoll);
    registerCommand("panic", (command*)panicTest);
    registerCommand("sector", (command*)readSectorTest);
    registerCommand("shutdown", (command*)shutdown);
    registerCommand("memory", (command*)memoryInfo);
    registerCommand("dump", (command*)dump);
    serialPrintf("All commands registered successfully.\n");
    printf("WARNING: The command line may be unstable, please be careful and double check your commands.\n");
    printf("I'm working on multiple other things right now - this is on the list, however.\n");
    printf("Command handler initialized - type 'help' for help.\n");

    char buffer[256]; // We will store keyboard input here.
    enableShell("reduceOS> "); // Enable a boundary (our prompt)


    while (true) {
        printf("reduceOS> ");
        keyboardGetLine(buffer);
        parseCommand(buffer);
    }
}