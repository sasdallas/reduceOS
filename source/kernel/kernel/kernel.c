// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/kernel.h> // Kernel header file
#include <kernel/cmds.h>



// ide_ata.c defined variables
extern ideDevice_t ideDevices[4];

// initrd.c defined variables
extern fsNode_t *initrdDev;

multiboot_info *globalInfo;

extern void switchToUserMode(); // User mode switch function



int syscall0() {
    printf("0 parameter - SUCCESS\n");
    return 0;
}

int syscall1(int p1) {
    if (p1 == 1) {
        printf("1 parameter - SUCCESS\n");
    }

    return 1;
}

int syscall2(int p1, int p2) {
    if (p1 == 1 && p2 == 2) {
        printf("2 parameter - SUCCESS\n");
    }

    return 2;
}

int syscall3(int p1, int p2, int p3) {
    if (p1 == 1 && p2 == 2 && p3 == 3) {
        printf("3 parameter - SUCCESS\n");
    }

    return 3;
}

int syscall4(int p1, int p2, int p3, int p4) {
    if (p1 == 1 && p2 == 2 && p3 == 3 && p4 == 4) {
        printf("4 parameter - SUCCESS\n");
    }

    return 4;
}

int syscall5(int p1, int p2, int p3, int p4, int p5) {
    if (p1 == 1 && p2 == 2 && p3 == 3 && p4 == 4 && p5 == 5) {
        printf("5 parameter - SUCCESS\n");
    }

    return 5;
}

int syscall6(int p1, int p2, int p3, int p4, int p5, int p6) {
    if (p1 == 1 && p2 == 2 && p3 == 3 && p4 == 4 && p5 == 5 && p6 == 6) {
        printf("6 parameter - SUCCESS\n");
    } else {
        printf("6 parameter - CALLED BUT NOT SUCCESS (%i %i %i %i %i %i).\n", p1, p2, p3, p4, p5, p6);
    }

    return 6;
}

void usermodeMain() {
    printf("Hello, usermode world! printf() is working!\n");
    int r1 = syscall_syscall0();
    int r2 = syscall_syscall1(1);
    int r3 = syscall_syscall2(1, 2);
    int r4 = syscall_syscall3(1, 2, 3);
    int r5 = syscall_syscall4(1, 2, 3, 4);
    int r6 = syscall_syscall5(1, 2, 3, 4, 5);
    int r7 = syscall_syscall6(1, 2, 3, 4, 5, 6);

    if (r1 == 0 && r2 == 1 && r3 == 2 && r4 == 3 && r5 == 4 && r6 == 5 && r7 == 6) {
        printf("All system calls are valid.\n");
    }

    for (;;);
}




int enterUsermode(int argc, char *args[]) {
    printf("Entering usermode, please wait...\n");
    setKernelStack(2048);
    switchToUserMode();
    return -1;
}



fsNode_t *fatDriver = NULL;
fsNode_t *ext2_root = NULL;



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
    for (int i = 0; i < (SCREEN_WIDTH - strlen("reduceOS v (created by @sasdallas) ") - strlen(CODENAME) - strlen(VERSION)); i++) printf(" "); // tbf
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

    // Before anything else is allocated, we need to deinitialize a few other regions.
    // Here is the list:
    /*
        - ACPI (0x000E0000 - 0x000FFFFF)
    */

    pmm_deinitRegion(0x000E0000, 0x000FFFFF-0x000E0000);

    updateBottomText("Initializing HAL...");
    cpuInit();
    printf("HAL initialization completed.\n");

    // Initialize ACPI (has to be done before VMM initializes)
    updateBottomText("Initializing ACPI...");
    acpiInit();

    // Initialize VMM
    vmmInit();
    serialPrintf("Initialized memory management successfully.\n");


    // TODO: ACPI might need to reinitialize its regions so do we need to reallocate? I call vmm_allocateRegion in ACPI, but does that work before vmmInit?

    
    ASSERT(globalInfo->m_modsCount > 0, "kmain", "Initial ramdisk not found (modsCount is 0)");
    uint32_t initrdLocation = *((uint32_t*)(globalInfo->m_modsAddr));
    uint32_t initrdEnd = *(uint32_t*)(globalInfo->m_modsAddr + 4);
    serialPrintf("GRUB did pass an initial ramdisk at 0x%x.\n", globalInfo->m_modsAddr);

    multiboot_mod_t *mod;
    int i;
    for (i = 0, mod = (multiboot_mod_t*)globalInfo->m_modsAddr; i < globalInfo->m_modsCount; i++, mod++) {
        serialPrintf("\tmod_start = 0x%x, mod_end = 0x%x, cmdline = %s\n", mod->mod_start, mod->mod_end, (char*)mod->cmdline);
    }


    // ONLY WORKS WHEN LOADING WITH GRUB. DOES NOT WORK ON QEMU.
    //fsNode_t *initrd = initrdInit(initrdLocation);    
    printf("Initrd image initialized!\n");
    serialPrintf("initrdInit: Initial ramdisk loaded - location is 0x%x and end address is 0x%x\n", initrdLocation, initrdEnd);
    


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

    
    // Probe for PCI devices (bugged on VBOX)
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("initPCI: PCI probe completed\n");

    


    

    
    // Initialize the floppy drive (bugged on VBOX)
    floppy_init();
    serialPrintf("Initialized floppy drive successfully.\n");


    
    // DEFINITELY sketchy! What's going on with this system? 
    serialPrintf("WARNING: Enabling liballoc! Stand away from the flames!\n");
    enable_liballoc();
    
    // Calculate the initial ramdisk location in memory (panic if it's not there)
    
    

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE
    

    /* VESA initialization */
    bool didInitVesa = true;
    char c = isKeyPressed();
    if (c == 'c') { 
        didInitVesa = false;
    } else {
        vesaInit(); // Initialize VBE
    }


    



    if (didInitVesa) {
        // Initialize PSF
        psfInit();

        vbeSwitchBuffers();
    
        changeTerminalMode(1); // Update terminal mode
    }


   

    // Initialize the VFS
    vfsInit();

    // First, we need to install the EXT2 and FAT drivers.
    ext2_install(0, NULL);
    fat_install(0, NULL);
    ide_install(0, NULL);

    // Then, we need to map the /dev/ directory
    vfs_mapDirectory("/dev");

    // Now, we can iterate through each IDE node, mount them to the dev directory, and try to use them as root if needed
    bool rootMounted = false;
    for (int i = 0; i < 4; i++) {
        fsNode_t *ideNode = ideGetVFSNode(i);

        // First, we'll mount this device to /dev/idex
        char *name = kmalloc(sizeof(char) * (strlen("/dev/ide")  + 1));
        strcpy(name, "/dev/ide");
        itoa(i, name + strlen("/dev/ide"), 10);
    
        
        vfsMount(name, ideGetVFSNode(i));

        if ((ideDevices[i].reserved == 1) && (ideDevices[i].size > 1)) {
            // The EXT2 driver needs to be mounted first on /
            if (!rootMounted) {
                int ret = vfs_mountType("ext2", name, "/");
                if (ret == 0) rootMounted = true;
                else {
                    // Other filesystem should initialize differently
                }
            } 
        }
    }

    debug_print_vfs_tree();

    // For compatibility with our tests, we need to set the ext2_root variable.
    // The user can use the mount_fat command to mount the FAT driver.
    if (rootMounted) ext2_root = open_file("/", 0);


    
    uint8_t seconds, minutes, hours, days, months;
    int years;

    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &years);
    serialPrintf("rtc_getDateTime: Got date and time from RTC (formatted as M/D/Y H:M:S): %i/%i/%i %i:%i:%i\n", months, days, years, hours, minutes, seconds);



    // Initialize system calls
    initSyscalls();

    // Start paging if VBE was not initialized.
    useCommands();
}



void useCommands() {
    serialPrintf("kmain: Memory management online with %i KB of physical memory\n", pmm_getPhysicalMemorySize());


    clearScreen(COLOR_WHITE, COLOR_CYAN);
    
    clearBuffer();


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
    registerCommand("user", (command*)enterUsermode);


    registerCommand("mount_fat", (command*)mountFAT);

    registerCommand("ls", (command*)ls);
    registerCommand("cd", (command*)cd);
    registerCommand("cat", (command*)cat);
    registerCommand("create", (command*)create);
    registerCommand("mkdir", (command*)mkdir);
    registerCommand("pwd", (command*)pwd);
    registerCommand("bitmap", (command*)show_bitmap);
    registerCommand("edit", (command*)edit);

    serialPrintf("kmain: All commands registered successfully.\n");
    serialPrintf("kmain: Warning: User is an unstable environment.\n");
    
    char buffer[256]; // We will store keyboard input here.
    printf("reduceOS has finished loading successfully.\n");
    printf("Please type your commands below.\n");
    printf("Type 'help' for help.\n");
    if (!fs_root) printf("WARNING: No root filesystem is mounted.\n");
    enableShell("reduceOS /> "); // Enable a boundary (our prompt)

    while (true) {
        printf(getShell());
        keyboardGetLine(buffer);
        parseCommand(buffer);
    }
}
