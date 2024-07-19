// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file


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
        uint32_t *addr = (uint32_t*)strtol(args[2], NULL, 16);

        printf("Value at memory address 0x%x: 0x%x (%i)\n", addr, *addr, (int)*addr);
    } else if (!strcmp(args[1], "memoryrange") && argc > 3) {
        printf("Warning: Dumping memory in the wrong spots can crash the OS.\n");
        uint32_t *addr1 = (uint32_t*)strtol(args[2], NULL, 16);
        uint32_t *addr2 = (uint32_t*)strtol(args[3], NULL, 16);
        printf("Values from memory addresses 0x%x - 0x%x:\n", addr1, addr2);
        for (addr1; addr1 < addr2; addr1 += 4) {
            printf("0x%x: ", addr1);
            uint8_t bytes[4];
            memcpy(bytes, addr1, sizeof(uint32_t));
            printf("0x%x 0x%x 0x%x 0x%x", bytes[0], bytes[1], bytes[2], bytes[3]);
            printf("\n");
        }
    } else if (!strcmp(args[1], "multiboot")) {
        printf("Multiboot information:\n");
        printf("m_flags: 0x%x\n", globalInfo->m_flags);
        printf("m_memoryLo: 0x%x\n", globalInfo->m_memoryLo);
        printf("m_memoryHi: 0x%x\n", globalInfo->m_memoryHi);
        printf("m_bootDevice: 0x%x\n", globalInfo->m_bootDevice);
        printf("m_cmdLine: %s\n", globalInfo->m_cmdLine);
        printf("m_modsCount: %d\n", globalInfo->m_modsCount);
        printf("m_modsAddr: 0x%x\n", globalInfo->m_modsAddr);
        printf("m_mmap_addr: 0x%x\n", globalInfo->m_mmap_addr);
        printf("m_mmap_length: 0x%x\n", globalInfo->m_mmap_length);
        printf("m_bootloader_name: %s\n", globalInfo->m_bootloader_name);
    } else {
        printf("Invalid arguments, please check if your syntax is correct.\n");
        printf("Possible arguments: sysinfo, memory <addr>, memoryrange <addr1> <addr2>, multiboot\n");
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

extern pciDevice **pciDevices;
extern int dev_idx;

int pciInfo(int argc, char *args[]) { 
    printPCIInfo();
    printf("Done executing\n");
    return 1;
}



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


int testISRException(int argc, char *args[]) {
    asm volatile ("div %0" :: "r"(0));
    printf("ISR fail\n");
    return 0;
}


int about(int argc, char *args[]) {
    printf("reduceOS v%s (codename %s)\n", VERSION, CODENAME);
    printf("Build number %s-%s - build date %s\n", BUILD_NUMBER, BUILD_CONFIGURATION, BUILD_DATE);
    printf("Created by @sasdallas\n");
}

int color(int argc, char *args[]) {
    if (argc != 3) {
        printf("Usage: color <fg> <bg>\n");
        printf("\"fg\" and \"bg\" are VGA color integers.\n");
        return 1;
    }

    if (terminalMode != 1) {
        printf("This command only works in VESA VBE mode.\n");
        return 1;
    }

    int fg = atoi(args[1]);
    int bg = atoi(args[2]);

    if (fg < 0 || fg > 15) {
        printf("Invalid foreground color.\n");
        return 1;
    }

    if (bg < 0 || bg > 15) {
        printf("Invalid background color.\n");
        return 1;
    }

    if (fg == bg) {
        printf("Colors must be different (else code breaks)\n");
        return 1;
    }

    instantUpdateTerminalColor(fg, bg);
    return 0;
}

// Used for debugging
int panicTest(int argc, char *args[]) {
    panic("kernel", "panicTest()", "Testing panic function");
}

int readFloppyTest(int sect) {
    // dont use for testing
    printf("Writing sector, first...\n");
    uint8_t *write_buffer = 0;
    memset(write_buffer, 0xFF, 512);
    floppy_writeSector(0, write_buffer);


    printf("Reading sector...\n");

    uint32_t sector = (uint32_t)sect;
    uint8_t *buffer = 0;

    int ret = floppy_readSector(sector, &buffer);
    
    if (ret != FLOPPY_OK) {
        printf("Could not read sector. Error code %i\n", ret);
        return -1;
    }

    printf("Contents of sector %i:\n", sector);

    if (buffer != 0) {
        int i = 0;
        for (int c = 0; c < 4; c++) {
            for (int j = 0; j < 128; j++) printf("0x%x ", buffer[i + j]);
            i += 128;

            printf("Press any key to continue.\n");
            keyboardGetChar();
        }
    }

    return 0;
}


int read_floppy(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: read_floppy <sector>\nThis command will read out a sector.");
        return -1;
    }

    uint32_t sector = (uint32_t)atoi(args[1]);
    uint8_t *buffer = 0;

    printf("Reading sector %i...\n", sector);

    int ret = floppy_readSector(sector, &buffer);
    
    if (ret != FLOPPY_OK) {
        printf("Could not read sector. Error code %i\n", ret);
        return -1;
    }

    printf("Contents of sector %i:\n", sector);

    if (buffer != 0) {
        int i = 0;
        for (int c = 0; c < 4; c++) {
            for (int j = 0; j < 128; j++) printf("0x%x ", buffer[i + j]);
            i += 128;

            printf("Press any key to continue.\n");
            keyboardGetChar();
        }
    }

    return 0;
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


int doPageFault(int argc, char *args[]) {
    // haha page fault
    uint32_t *fault = (uint32_t*)0xF0000000;
    uint32_t dofault = *fault;

    printf("what\n");
    return -1;
}


int serviceBIOS32(int argc, char *args[]) {
    printf("Serving INT 0x15...\n");
    REGISTERS_16 in, out = {0};

    in.ax = 0xE820;
    bios32_call(0x15, &in, &out);

    printf("Interrupt serviced. Results:\n");
    printf("AX = 0x%x BX = 0x%x CX = 0x%x DX = 0x%x\n", out.ax, out.bx, out.cx, out.dx);
}



int memoryInfo(int argc, char *args[]) {
    printf("Available physical memory: %i KB\n", globalInfo->m_memoryHi - globalInfo->m_memoryLo);
    return 0;
}

// This function is hopefully going to serve as a function to test multiple modules of the OS.
int test(int argc, char *args[]) {
    if (argc != 2) { printf("Usage: test <module>\n"); return -1; }

    if (!strcmp(args[1], "pmm")) {
        printf("=== TESTING PHYSICAL MEMORY MANAGEMENT ===\n");
        void *a = pmm_allocateBlock();
        printf("\tAllocated 1 block to a to 0x%x\n", a);
        
        void *b = pmm_allocateBlocks(2);
        printf("\tAllocated 2 blocks to b at 0x%x\n", b);

        void *c = pmm_allocateBlock();
        printf("\tAllocated 1 block to c to 0x%x\n", c);

        pmm_freeBlock(a);
        pmm_freeBlock(c);

        c = pmm_allocateBlock();
        a = pmm_allocateBlock();

        printf("\tFreed a and c\n");
        printf("\tReallocated c to 0x%x\n", c);
        printf("\tReallocated a to 0x%x\n", a);

        pmm_freeBlocks(b, 2);
        printf("\tFreed 2 blocks of b\n");
        
        void *d = pmm_allocateBlock();
        printf("\tAllocated d to 0x%x\n", d);

        pmm_freeBlock(a);
        pmm_freeBlock(c);
        pmm_freeBlock(d);

        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "liballoc")) {
        printf("=== TESTING LIBALLOC ===\n");
        void *a = kmalloc(8);
        printf("\tAllocated 8 bytes to a to 0x%x\n", a);
        
        void *b = kmalloc(16);
        printf("\tAllocated 16 bytes to b at 0x%x\n", b);

        void *c = kmalloc(8);
        printf("\tAllocated 8 bytes to c to 0x%x\n", c);

        kfree(a);
        kfree(c);

        c = kmalloc(8);
        a = kmalloc(8);

        printf("\tFreed a and c\n");
        printf("\tReallocated c to 0x%x\n", c);
        printf("\tReallocated a to 0x%x\n", a);

        kfree(b);
        printf("\tFreed 16 bytes of b\n");
        
        void *d = kmalloc(8);
        printf("\tAllocated 8 bytes to d to 0x%x\n", d);

        kfree(a);
        kfree(c);
        kfree(d);
        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "bios32")) {
        printf("=== TESTING BIOS32 ===\n");

        printf("\tServing INT 0x15...\n");
        REGISTERS_16 in, out = {0};

        in.ax = 0xE820;
        bios32_call(0x15, &in, &out);

        printf("\tInterrupt serviced. Results:\n");
        printf("\tAX = 0x%x BX = 0x%x CX = 0x%x DX = 0x%x\n", out.ax, out.bx, out.cx, out.dx);

        printf("=== TESTS COMPLETED ===\n");
    } else {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32\n");
    }

    return 0;
}

void usermodeMain() {

}

// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(unsigned long addr, unsigned long loader_magic) {
    // Update global multiboot info.

    multiboot_info *mboot;

    globalInfo = (multiboot_info*)addr;
    
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
    printf("reduceOS v%s %s (created by @sasdallas)", VERSION, CODENAME);
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v1.0 (created by @sasdallas) ") - strlen(CODENAME)); i++) printf(" "); // tbf
    // Next, update terminal color to the proper color.
    updateTerminalColor(vgaColorEntry(COLOR_WHITE, COLOR_CYAN));
    printf("reduceOS is loading, please wait...\n");

    // Change the bottom text of the terminal (updateBottomText)
    updateBottomText("Loading reduceOS...");

    // Initialize serial logging
    serialInit();
    serialPrintf("============================================================================================================================\n");
    serialPrintf("\treduceOS v%s %s - written by sasdallas\n", VERSION, CODENAME);
    serialPrintf("\tNew kernel, same trash.\n");
    serialPrintf("\tBuild %s-%s, compiled on %s\n", BUILD_NUMBER, BUILD_CONFIGURATION, BUILD_DATE);
    serialPrintf("============================================================================================================================\n\n");
    serialPrintf("Kernel location: 0x%x - 0x%x\nText section: 0x%x - 0x%x; Data section: 0x%x - 0x%x; BSS section: 0x%x - 0x%x\n", &text_start, &bss_end, &text_start, &text_end, &data_start, &data_end, &bss_start, &bss_end);
    serialPrintf("Loader magic: 0x%x\n\n", loader_magic);
    serialPrintf("Serial logging initialized!\n");
    

    printf("Serial logging initialized on COM1.\n");



    if (loader_magic != 0x43D8C305) {
        serialPrintf("loader magic: 0x%x addr: 0x%x\n", loader_magic, addr);
        panic("kernel", "kmain", "loader_magic != 0x43D8C305"); 
    }



    updateBottomText("Initializing HAL...");
    cpuInit();
    printf("HAL initialization completed.\n");

    
    serialPrintf("kmain: CPU has long mode support: %i\n", isCPULongModeCapable());

    // ==== MEMORY MANAGEMENT INITIALIZATION ==== 
    // Now that initial ramdisk has loaded, we can initialize memory management
    updateBottomText("Initializing physical memory manager...");
    pmmInit((globalInfo->m_memoryHi - globalInfo->m_memoryLo));

    // Initialize the memory map
    pmm_initializeMemoryMap(globalInfo);

    // Deallocate the kernel's region.
    uint32_t kernelStart = (uint32_t)&text_start;
    uint32_t kernelEnd = (uint32_t)&bss_end;
    pmm_deinitRegion(0x100000, (kernelEnd - kernelStart));

    // Initialize VMM
    vmmInit();
    serialPrintf("Initialized memory management successfully.\n");

   

    bios32_init();
    serialPrintf("bios32 initialized successfully!\n");
    

    
    // Initialize Keyboard 
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    setKBHandler(true);
    serialPrintf("Keyboard handler initialized.\n");
    

    // PIT timer
    updateBottomText("Initializing PIT...");
    pitInit();
    serialPrintf("PIT started at 1000hz\n");

    
    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("initPCI: PCI probe completed\n");

    // Initialize the VFS
    vfsInit();

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE
    
    // Initialize ACPI
    updateBottomText("Initializing ACPI...");
    acpiInit();

    // Calculate the initial ramdisk location in memory (panic if it's not there)
    ASSERT(globalInfo->m_modsCount > 0, "kmain", "Initial ramdisk not found (modsCount is 0)");
    uint32_t initrdLocation = *((uint32_t*)(globalInfo->m_modsAddr));
    uint32_t initrdEnd = *(uint32_t*)(globalInfo->m_modsAddr + 4);
    serialPrintf("GRUB did pass an initial ramdisk at 0x%x.\n", globalInfo->m_modsAddr);

    // Bugged initrd driver.
    //fs_root = initrdInit(initrdLocation);    
    printf("Initrd image initialized!\n");
    serialPrintf("initrdInit: Initial ramdisk loaded - location is 0x%x and end address is 0x%x\n", initrdLocation, initrdEnd);

    

    
    // Initialize the floppy drive
    floppy_init();
    serialPrintf("Initialized floppy drive successfully.\n");



    // Finally, all setup has been completed. We can ask the user if they want to enter the backup command line.
    // By ask, I mean check if they're holding c.

    bool didInitVesa = true;
    char c = isKeyPressed();
    if (c == 'c') { 
        didInitVesa = false;
    } else {
        vesaInit(); // Initialize VBE
        bitmapFontInit();
    }

    fatInit(); // BUG: FAT can only be initialized after VESA, apparently. Don't ask.



    if (didInitVesa) {
        // Initialize PSF
        psfInit();
        
        vbeSwitchBuffers();
    }


    // DEFINITELY sketchy! What's going on with this system? Why can't
    serialPrintf("WARNING: Enabling liballoc! Stand away from the flames!\n");
    enable_liballoc();


    uint8_t seconds, minutes, hours, days, months;
    int years;

    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &years);
    serialPrintf("rtc_getDateTime: Got date and time from RTC (formatted as M/D/Y H:M:S): %i/%i/%i %i:%i:%i\n", months, days, years, hours, minutes, seconds);


    // Start paging if VBE was not initialized.
    useCommands();
}



void useCommands() {
    clearScreen(COLOR_WHITE, COLOR_CYAN);
    clearBuffer();

    printf("Memory management online with %i KB of physical memory\n", pmm_getPhysicalMemorySize());


    // The user entered the command handler. We will not return.

    initCommandHandler();
    registerCommand("isr", (command*)testISRException);
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
    registerCommand("about", (command*)about);
    registerCommand("version", (command*)about);
    registerCommand("color", (command*)color);
    registerCommand("pagefault", (command*)doPageFault);
    registerCommand("read_floppy", (command*)read_floppy);
    registerCommand("test", (command*)test);


    serialPrintf("All commands registered successfully.\n");
    serialPrintf("Warning: User is an unstable environment.\n");

    
    char buffer[256]; // We will store keyboard input here.
    printf("reduceOS has finished loading successfully.\n");
    printf("Please type your commands below.\n");
    printf("Type 'help' for help.\n");
    enableShell("reduceOS> "); // Enable a boundary (our prompt)
    
    

    while (true) {
        printf("reduceOS> ");
        keyboardGetLine(buffer);
        parseCommand(buffer);
    }
}