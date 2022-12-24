// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file



int testFunction(char *args[]) {
    printf("It works!\n");
    return 1;
}


int getSystemInformation(char *args[]) {
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


int help(char *args[]) {
    printf("reduceOS v1.0-dev\nValid commands:\ntest, system, help\n");
    return 1;
}

// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(multiboot_info* mem) {
    // Get kernel size
    extern uint32_t end;
    uint32_t kernelSize = &end;

    

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
    
    // Initialize IDT.
    updateBottomText("Initializing IDT...");
    idtInit(0x8);
    

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

    // Quickly inform the user of their available memory. (note: yes, I know 1 GB is equal 1000 MB and 1 MB is equal to 1000 KB, but for some reason it breaks things so I don't care)

    int memoryGB = mem->m_memorySize / 1024 / 1024; 
    int memoryMB = mem->m_memorySize / 1024 - (memoryGB * 1024);
    int memoryKB = mem->m_memorySize - (memoryMB * 1024);
    printf("Available memory: %i GB %i MB %i KB\n", memoryGB, memoryMB, memoryKB);

    // PIT timer
    updateBottomText("Initializing PIT...");
    i86_pitInit();

    // Initialize Keyboard (won't work until interrupts are enabled - that's when IRQ and stuff is enabled.)
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    enableKBHandler(true);


    // Enable hardware interrupts
    updateBottomText("Enabling interrupts...");
    enableHardwareInterrupts();
    printf("Interrupts enabled.\n");

    
    
    // Initalize physical memory.

    updateBottomText("Initializing physical memory management...");

    
    printf("Kernel size: 0x%x (%i bytes)\n", kernelSize, kernelSize);


    // Initialize physical memory manager.
    // Place the memory bitmap used by PMM at the end of the kernel in memory.
    
    
    // NEXT PATCH: Incorporating heap.c, paging.c, and figuring out what to do with physical_memory.c
    
    printf("reduceOS 1.0-dev has completed basic initialization.\nThe command line is now enabled. Type 'help' for help!\n");

    initCommandHandler();

    registerCommand("test", (command*)testFunction);
    registerCommand("system", (command*)getSystemInformation);
    registerCommand("help", (command*)help);

    char buffer[256]; // We will store keyboard input here.

    while (true) {
        keyboardGetLine(buffer, sizeof(buffer));
        parseCommand(buffer);
    }
}
