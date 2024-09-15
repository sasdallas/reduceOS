// =====================================================================
// kernel.c - Main reduceOS kernel
// This file handles most of the logic and puts everything together
// =====================================================================
// This file is apart of the reduceOS C kernel. Please credit me if you use this code.

#include <libk_reduced/stdint.h>
#include <libk_reduced/stdio.h>
#include <libk_reduced/time.h>

#include <kernel/acpi.h>
#include <kernel/args.h>
#include <kernel/clock.h>
#include <kernel/cmds.h>
#include <kernel/command.h>
#include <kernel/console.h>
#include <kernel/debugdev.h>
#include <kernel/dma.h>
#include <kernel/elf.h>
#include <kernel/ext2.h>
#include <kernel/fat.h>
#include <kernel/floppy.h>
#include <kernel/font.h>
#include <kernel/ide_ata.h>
#include <kernel/kernel.h> // Kernel header file
#include <kernel/keyboard.h>
#include <kernel/ksym.h>
#include <kernel/modfs.h>
#include <kernel/module.h>
#include <kernel/nulldev.h>
#include <kernel/pci.h>
#include <kernel/process.h>
#include <kernel/processor.h>
#include <kernel/serialdev.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <kernel/tarfs.h>
#include <kernel/terminal.h>
#include <kernel/test.h>
#include <kernel/ttydev.h>
#include <kernel/vesa.h>
#include <kernel/video.h>

// ide_ata.c defined variables
extern ideDevice_t ideDevices[4];

// initrd.c defined variables
extern fsNode_t* initrdDev;

multiboot_info* globalInfo;

extern void switchToUserMode(); // User mode switch function
void kshell();

void usermodeMain() {
    printf("Hello!\n");
    // Implicit function declaration. Need to know how to solve
    // syscall_sys_write(2, "Hello, system call world!", strlen("Hello, system call world!"));

    for (;;);
}

int enterUsermode(int argc, char* args[]) {
    printf("Entering usermode, please wait...\n");
    setKernelStack(2048);
    switchToUserMode();
    return -1;
}

// Filesystem nodes
fsNode_t* fatDriver = NULL;
fsNode_t* ext2_root = NULL;

// Function prototypes
void useCommands();
void kshell();

// kmain() - The most important function in all of reduceOS. Jumped here by loadKernel.asm.
void kmain(unsigned long addr, unsigned long loader_magic) {
    // Update global multiboot info.
    globalInfo = (multiboot_info*)addr;

    // Some extra stuff
    extern uint32_t text_start;
    extern uint32_t text_end;
    extern uint32_t data_start;
    extern uint32_t data_end;
    extern uint32_t bss_start;
    extern uint32_t bss_end;

    // ==== MEMORY MANAGEMENT INITIALIZATION ====
    // This has to be done before ANYTHING else, because lack of PMM will break a lot of stuff.
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

    pmm_deinitRegion(0x000E0000, 0x000FFFFF - 0x000E0000);

    // Even though paging hasn't been initialized, liballoc can use PMM blocks.
    enable_liballoc();

    // Initialize PC Screen Font for later
    psfInit();

    // Initialize serial logging
    serialInit();
    serialPrintf("====================================================================================================="
                 "=======================\n");
    serialPrintf("\treduceOS v%s %s - written by sasdallas\n", VERSION, CODENAME);
    serialPrintf("\tNew kernel, same trash.\n");
    serialPrintf("\tBuild %s-%s, compiled on %s\n", BUILD_NUMBER, BUILD_CONFIGURATION, BUILD_DATE);
    serialPrintf("====================================================================================================="
                 "=======================\n\n");
    serialPrintf("Kernel location: 0x%x - 0x%x\nText section: 0x%x - 0x%x; Data section: 0x%x - 0x%x; BSS section: "
                 "0x%x - 0x%x\n",
                 &text_start, &bss_end, &text_start, &text_end, &data_start, &data_end, &bss_start, &bss_end);
    serialPrintf("Loader magic: 0x%x\n\n", loader_magic);
    serialPrintf("Serial logging initialized!\n");

    if (loader_magic != 0x43D8C305) {
        serialPrintf("loader magic: 0x%x addr: 0x%x\n", loader_magic, addr);
        panic("kernel", "kmain", "loader_magic != 0x43D8C305");
    }

    // Before CPU initialization can even take place, we should start the clock.
    clock_init();

    // Now initialize ACPI, has to be done before VMM (it will allocate regions I think)
    acpiInit();

    // Initialize all the basic CPU features
    updateBottomText("Initializing HAL...");
    cpuInit();
    printf("HAL initialization completed.\n");

    // Initialize VMM
    updateBottomText("Initializing virtual memory management...");
    vmmInit();
    serialPrintf("Initialized memory management successfully.\n");

    // Deinitialize multiboot modules
    multiboot_mod_t* mod;
    uint32_t i2;
    for (i2 = 0, mod = (multiboot_mod_t*)globalInfo->m_modsAddr; i2 < globalInfo->m_modsCount; i2++, mod++) {
        pmm_deinitRegion(mod->mod_start, mod->mod_end - mod->mod_start);
    }

    // While we're on the topic of multiboot, setup the argument parser
    args_init((char*)globalInfo->m_cmdLine);

    // TODO: ACPI might need to reinitialize its regions so do we need to reallocate? I call vmm_allocateRegion in ACPI, but does that work before vmmInit

    // Installs the GDT and IDT entries for BIOS32
    bios32_init();
    serialPrintf("bios32 initialized successfully!\n");

    // ==== TERMINAL INITIALIZATION ====
    video_init();

    initTerminal();

    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color
    terminalUpdateTopBarKernel("created by @sasdallas");

    // Next, update terminal color to the proper color and print loading text.
    updateTerminalColor_gfx(COLOR_WHITE, COLOR_CYAN);

    printf("reduceOS is loading, please wait...\n");

    // ==== PERIPHERAL/DRIVER INITIALIZATION ====

    // PIT timer
    updateBottomText("Initializing PIT...");
    pitInit();

    // Initialize Keyboard
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    setKBHandler(true);
    serialPrintf("Keyboard handler initialized.\n");

    // Setup the DMA pool
    // I can hear the page fault ready to happen because DMA wasn't properly allocated.
    dma_initPool(256 * 1024); // 256 KB of DMA pool

    // Initialize the floppy drive (will do IRQ waiting loop occasionally - NEED to fix)
    floppy_init();
    serialPrintf("Initialized floppy drive successfully.\n");

    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("initPCI: PCI probe completed\n");

    // Initialize debug symbols (note that this just allocates memory for the structures)
    ksym_init();

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE

    // Start the process scheduler
    scheduler_init();
    printf("Process scheduler initialized.\n");

    // ==== FILESYSTEM INITIALIZATION ====

    // Initialize the VFS
    vfsInit();

    // First, we need to install some filesystem drivers.
    printf("Preparing filesystem drivers...");
    ext2_install(0, NULL);
    fat_install(0, NULL);
    ide_install(0, NULL);
    tar_install();
    printf("done\n");

    // Then, we need to map the /device/ directory
    vfs_mapDirectory("/device");

    // Mount the other devices
    printf("Preparing devices...");
    nulldev_init();         // /device/null
    zerodev_init();         // /device/zero
    serialdev_init();       // /device/serial/COMx
    modfs_init(globalInfo); // /device/modules/modx
    console_init();         // /device/console

    fsNode_t* comPort = open_file("/device/serial/COM1", 0);
    debugdev_init(comPort); // /device/debug

    console_setOutput(printf_output);
    printf("done\n");

    serial_changeCOM(SERIAL_COM1); // Bochs bugs out without this

    // Try to find the initial ramdisk (sorry about the messy code)
    int i = 0;
    fsNode_t* initrd = NULL;
    while (true) {
        char* mntpoint = kmalloc(strlen("/device/modules/modx"));
        strcpy(mntpoint, "/device/modules/mod");
        itoa((void*)i, mntpoint + strlen("/device/modules/mod"), 10);

        fsNode_t* modnode = open_file(mntpoint, 0);
        if (!modnode) {
            kfree(mntpoint);
            break;
        }

        multiboot_mod_t* mod = (multiboot_mod_t*)modnode->impl_struct;
        if (strstr((char*)mod->cmdline, "type=initrd") != NULL) {
            modnode->close(modnode);
            initrd = open_file(mntpoint, 0);
            kfree(mntpoint);
            kfree(modnode);
            break;
        }

        kfree(mntpoint);
        kfree(modnode);
        i++;
    }

    if (initrd == NULL) { panic("kernel", "kmain", "Initial ramdisk not found."); }

    printf("Located initial ramdisk successfully.\n");

    // Now, we can iterate through each IDE node, mount them to the dev directory, and try to use them as root if needed
    bool rootMounted = false;
    for (int i = 0; i < 4; i++) {
        fsNode_t* ideNode = ideGetVFSNode(i);

        if ((int)ideNode->impl == -1) {
            // Drive does not exist, but memory for the fsNode was still allocated
            kfree(ideNode);
            continue;
        }

        // First, we'll mount this device to /device/idex
        char* name = kmalloc(sizeof(char) * (strlen("/device/ide") + 1));
        strcpy(name, "/device/ide");
        itoa((void*)i, name + strlen("/device/ide"), 10);

        vfsMount(name, ideNode);

        if ((ideDevices[i].reserved == 1) && (ideDevices[i].size > 1)) {
            // The EXT2 driver needs to be mounted first on /
            if (!rootMounted) {
                int ret = vfs_mountType("ext2", name, "/");
                if (ret == 0)
                    rootMounted = true;
                else {
                    // Other filesystem should initialize differently
                }
            }
        }
    }

    // We also need to mount the VESA VBE block device.
    vesa_createVideoDevice("fb0");

    printf("Mounted IDE nodes successfully.\n");

    // For compatibility with our tests, we need to set the ext2_root variable.
    // The user can use the mount_fat command to mount the FAT driver.
    if (rootMounted) ext2_root = open_file("/", 0);

    // If the user did not mount a drive, remount initial ramdisk to /.
    fsNode_t* debugsymbols;
    if (!rootMounted) {
        int ret = vfs_mountType("tar", initrd->name, "/");
        if (ret != 0) { panic("kernel", "kmain", "Failed to initialize initrd (tarfs init fail)"); }
        debugsymbols = open_file("/kernel.map", 0);
    } else {
        int ret = vfs_mountType("tar", initrd->name, "/device/initrd");
        if (ret != 0) { panic("kernel", "kmain", "Failed to initialize initrd (tarfs init fail)"); }
        debugsymbols = open_file("/device/initrd/kernel.map", 0);
    }

    // Bind debug symbols. REQUIRED for ELF loading.
    if (debugsymbols) {
        ksym_bind_symbols(debugsymbols);
    } else {
        panic("kernel", "kmain", "Failed to get kernel symbols!");
    }

    printf("Debug symbols loaded.\n");

    debug_print_vfs_tree(false);

    // ==== FINAL INITIALIZATION ====

    uint8_t seconds, minutes, hours, days, months;
    int years;

    rtc_getDateTime(&seconds, &minutes, &hours, &days, &months, &years);
    serialPrintf("rtc_getDateTime: Got date and time from RTC (formatted as M/D/Y H:M:S): %i/%i/%i %i:%i:%i\n", months,
                 days, years, hours, minutes, seconds);

    // Initialize system calls
    initSyscalls();

    // Start the module system
    module_init();

    // Scan and initialize modules for kernelspace (no more command handler)
    printf("Starting up modules...\n");
    module_parseCFG();

    printf("Kernel loading completed.\n");
    useCommands();
}

void useCommands() {
    serialPrintf("kmain: Memory management online with %i KB of physical memory\n", pmm_getPhysicalMemorySize());

    // Make sure the keyboard buffer is clear
    keyboard_clearBuffer();

    // Modules may want to use registerCommand
    printf("Preparing command handler...\n");
    initCommandHandler();

    printf("Finishing up...\n");

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
    registerCommand("clear", (command*)clear);
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
    registerCommand("rm", (command*)rm);

    registerCommand("strace", (command*)strace);
    registerCommand("pmm", (command*)pmm);
    registerCommand("vfs", (command*)vfs);
    registerCommand("load_elf", (command*)loadELF);
    registerCommand("mount_tar", (command*)mountTAR);
    registerCommand("modload", (command*)loadModule);
    registerCommand("start_process", (command*)makeProcess);
    registerCommand("start_thread", (command*)startThread);
    registerCommand("modinfo", (command*)modinfo);
    registerCommand("showmodes", (command*)showmodes);
    registerCommand("setmode", (command*)setmode);

    serialPrintf("kmain: All commands registered successfully.\n");

    printf("reduceOS has finished loading successfully.\n");
    printf("Please type your commands below.\n");
    printf("Type 'help' for help.\n");
    if (args_has("--force_vga"))
        printf("WARNING: You are currently in VGA text mode. This mode is deprecated and unsupported!\n");
    if (!strcmp(fs_root->name, "tarfs"))
        printf("WARNING: No root filesystem was mounted. The initial ramdisk has been mounted as root.\n");

    // Startup tasking, signals, and TTY
    tasking_start();
    signal_init();
    tty_init();

    // Try to start the init process, if available
    //char *argv[] = {"/init"};
    //system("/init", 1, argv);

    kshell();
}

void kshell() {
    serialPrintf("kmain: Shell ready\n");

    char buffer[256];            // We will store keyboard input here.
    enableShell("reduceOS /> "); // Enable a boundary (our prompt)

    while (true) {
        printf(getShell());
        keyboardGetLine(buffer);
        parseCommand(buffer);
        updateShell();
    }
}
