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
    printf("CPU frequency: %u\n", getCPUFrequency());

    return 1; // Return
}


int help(int argc, char *args[]) {
    printf("reduceOS v1.0\nValid commands:\nsystem, help, echo, pci, crash, anniversary, shutdown, initrd, ata, sector, task, ps\n");
    return 1;
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
    sleep(1000);
    printf("FOR SCIENCE, OF COURSE!");
    sleep(1000);
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

    // char buffer[256] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!'};
    // ideWriteSectors(drive, 1, 0, (uint32_t)buffer);
    // serialPrintf("Data written successfully.\n");


    // write message to drive
    const uint32_t LBA = 0;
    const uint8_t NO_OF_SECTORS = 1;
    char buf[512];

    memset(buf, 0, sizeof(buf));

    // write message to drive
    strcpy(buf, "Hello, world!");
    ideWriteSectors((int)drive, NO_OF_SECTORS, LBA, (uint32_t)buf);
    printf("Wrote data.\n");
    
    // read message from drive
    memset(buf, 0, sizeof(buf));
    ideReadSectors((int)drive, NO_OF_SECTORS, LBA, (uint32_t)buf);
    printf("Read data: %s\n", buf);
    return 0;
}







// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(multiboot_info* mem) {
    // Get build date and time.
    extern char BUILD_DATE;
    extern char BUILD_TIME;
    
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
    serialPrintf("Build date: %u, build time: %u\n", &BUILD_DATE, &BUILD_TIME);
    serialPrintf("Kernel location: 0x%x - 0x%x\nText section: 0x%x - 0x%x; Data section: 0x%x - 0x%x; BSS section: 0x%x - 0x%x\n", kernelStart, kernelEnd, text_start, text_end, data_start, data_end, bss_start, bss_end);
    serialPrintf("Serial logging initialized!\n");
    printf("Serial logging initialized on COM1.\n");



    

    updateBottomText("Initializing CPU...");
    cpuInit();
    printf("CPU initialized succesfully.\n");


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

    
    

    
    
   
    printf("Kernel is loaded at 0x%x, ending at 0x%x\n", kernelStart, kernelEnd);

    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("PCI probe completed\n");

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE
    
    
    // Initialize paging
    
    // Calculate the initial ramdisk location in memory (panic if it's not there)
    ASSERT(mem->m_modsCount > 0, "kmain", "Initial ramdisk not found (modsCount is 0)");
    uint32_t initrdLocation = *((uint32_t*)mem->m_modsAddr);
    uint32_t initrdEnd = *(uint32_t*)(mem->m_modsAddr + 4);
    placement_address = initrdEnd;
    serialPrintf("GRUB did pass an initial ramdisk.\n");


    updateBottomText("Initializing paging and heap...");
    initPaging(0xCFFFF000);
    serialPrintf("Paging and kernel heap initialized successfully (address: 0xCFFFF000)\n");
    
    fs_root = initrdInit(initrdLocation);    
    printf("Initrd image initialized!\n");
    serialPrintf("Initial ramdisk loaded - location is 0x%x and end address is 0x%x\n", initrdLocation, initrdEnd);


    initCommandHandler();

    anniversaryRegisterCommands();
    anniversaryRegisterEasterEggs();
    registerCommand("system", (command*)getSystemInformation);
    registerCommand("help", (command*)help);
    registerCommand("echo", (command*)echo);
    registerCommand("crash", (command*)crash);
    registerCommand("pci", (command*)pciInfo);
    registerCommand("initrd", (command*)getInitrdFiles);
    registerCommand("ata", (command*)ataPoll);
    registerCommand("panic", (command*)panicTest);
    registerCommand("sector", (command*)readSectorTest);
    registerCommand("shutdown", (command*)shutdown);
    
    serialPrintf("All commands registered successfully.\n");


    uint8_t seconds, minutes, hours, days, months;
    int years;

    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &years);
    serialPrintf("Got date and time from RTC (formatted as M/D/Y H:M:S): %i/%i/%i %i:%i:%i\n", months, days, years, hours, minutes, seconds);

    //updateBottomText("Initializing semaphore...");
    //sem_init(&sem, 1);

    //updateBottomText("Initializing multitasking (task #0)...");
    //initMultitasking();
    //printf("Multitasking initialized.\n");
    
    // Bugged function, cannot call.
    // acpiInit();

    printf("reduceOS 1.0-dev has completed basic initialization.\nThe command line is now enabled. Type 'help' for help!\n");

    char buffer[256]; // We will store keyboard input here.
    enableShell("reduceOS> "); // Enable a boundary (our prompt)


    while (true) {
        printf("reduceOS> ");
        keyboardGetLine(buffer);
        parseCommand(buffer);
        
    }
}
