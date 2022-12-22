// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include "include/kernel.h" // Kernel header file







// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(multiboot_info* mem) {
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

    
    // Initialize physical memory manager.
    // Place the memory bitmap used by PMM at the end of the kernel in memory.
    physMemoryInit(mem->m_memorySize, 0xC0000000);

    // Allocate regions 
    memoryRegion* region = (memoryRegion*)0x1000;
    for (int i =0; i < 10; i++) {
        if (region[i].type > 4) break; // Invalid type.
        if (i > 0 && region[i].startLo == 0) break; // Invalid region start.

        initRegion(region[i].startLo, region[i].sizeLo); // Initialize the region.
    }

    // Deinitialize the region the kernel is in.
    deinitRegion(0x1100, 0);

    // Possible bug here in that used blocks isn't a valid number.
    printf("regions initialized: %i; used blocks: %i; free blocks: %i\n", getBlockCount(), getUsedBlockCount(), getFreeBlockCount());
    
    char buffer[256]; // We will store keyboard input here.

    while (true) {
        keyboardGetLine(buffer);
        for (int i = 0; i < sizeof(buffer); i++) printf("%c", buffer[i]); // You can't print an array with %s in printf.
        for (int i = 0; i < sizeof(buffer); i++) if (buffer[i] == '\n') buffer[i] = '\0'; // Remove newline characters
        

        // TODO: Figure out how to parse commands.
    }
}