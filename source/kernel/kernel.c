// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file


// Linker defined symbols.
extern char *BUILD_DATE;
extern char *BUILD_TIME;

// heap.c defined variables.
extern uint32_t placement_address;


int testFunction(int argc, char *args[]) {
    printf("It works!\n");
    return 1;
}

int pagingTest(int argc, char *args[]) {
    printf("Executing paging test...\n");
    uint32_t a = kmalloc(8);
    uint32_t b = kmalloc(8);
    uint32_t c = kmalloc(8);
    
    printf("a: 0x");
    printf_hex(a);
    printf(", b: 0x");
    printf_hex(b);
    printf("\nc: 0x");
    printf_hex(c);

    kfree(c);
    kfree(b);
    uint32_t d = kmalloc(12);
    printf(", d: 0x");
    printf_hex(d);
    printf("\n");
    kfree(a);
    kfree(d);
    printf("Test complete.\n");
}


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

    return 1; // Return
}


int help(int argc, char *args[]) {
    printf("reduceOS v1.0-dev\nValid commands:\ntest, system, help, echo, pci, crash\n");
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


// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(multiboot_info* mem) {
    // Get kernel size
    extern uint32_t start;
    uint32_t kernelStart = &start;
    extern uint32_t end;
    uint32_t kernelEnd = &end;
    
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
    serialPrintf("Serial logging initialized!\n");
    printf("Serial logging initialized on COM1.\n");

    // Initialize GDT.
    updateBottomText("Initializing GDT...");
    gdtInit();
    
    // Initialize IDT.
    updateBottomText("Initializing IDT...");
    idtInit(0x8);
    
    serialPrintf("GDT and IDT have been initialized successfully.\n");

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

    // Quickly inform the user of their available memory. (note: yes, I know 1 GB is equal 1000 MB and 1 MB is equal to 1000 KB, but for some reason it breaks things so I don't care)

    // NOTE: Scrapped as of now - GRUB didn't pass us the amount of memory we have.

    // PIT timer
    updateBottomText("Initializing PIT...");
    i86_pitInit();
    serialPrintf("PIT started at 100hz\n");

    // Initialize Keyboard (won't work until interrupts are enabled - that's when IRQ and stuff is enabled.)
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    setKBHandler(true);
    serialPrintf("Keyboard handler initialized.\n");

    // Enable hardware interrupts
    updateBottomText("Enabling interrupts...");
    enableHardwareInterrupts();
    printf("Interrupts enabled.\n");
    serialPrintf("sti instruction did not fault - interrupts enabled.\n");
    
   
    printf("Kernel is loaded at 0x%x, ending at 0x%x\n", kernelStart, kernelEnd);

    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();

    // Initialize paging
    
    // Calculate the initial ramdisk location in memory (panic if it's not there)
    ASSERT(mem->m_modsCount > 0, "kmain", "Initial ramdisk not found (m_modsCount is 0)");
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

    printf("reduceOS 1.0-dev has completed basic initialization.\nThe command line is now enabled. Type 'help' for help!\n");

    initCommandHandler();

    anniversaryRegisterCommands();
    registerCommand("test", (command*)testFunction);
    registerCommand("system", (command*)getSystemInformation);
    registerCommand("help", (command*)help);
    registerCommand("echo", (command*)echo);
    registerCommand("crash", (command*)crash);
    registerCommand("pci", (command*)pciInfo);
    registerCommand("paging", (command*)pagingTest);
    registerCommand("initrd", (command*)getInitrdFiles);

    serialPrintf("All commands registered successfully.\n");

    char buffer[256]; // We will store keyboard input here.
    enableShell("reduceOS> "); // Enable a boundary (our prompt)


    while (true) {
        printf("reduceOS> ");
        keyboardGetLine(buffer, sizeof(buffer));
        parseCommand(buffer);
    }
}
