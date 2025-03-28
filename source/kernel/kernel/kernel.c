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
#include <kernel/hal.h>
#include <kernel/ide_ata.h>
#include <kernel/kernel.h>
#include <kernel/keyboard.h>
#include <kernel/ksym.h>
#include <kernel/modfs.h>
#include <kernel/module.h>
#include <kernel/nulldev.h>
#include <kernel/pci.h>
#include <kernel/process.h>
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

multiboot_info* globalInfo;     // Global multiboot info. I don't like.
uint32_t kernel_boot_time = 0;  // Bragging rights

extern void switchToUserMode(); // User mode switch function
void kshell();

void usermodeMain() {
    printf("Hello!\n");
    // Implicit function declaration. Need to know how to solve
    // syscall_sys_write(2, "Hello, system call world!", strlen("Hello, system call world!"));

    for (;;);
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
    mem_init();
    enable_liballoc(); // TODO: Remove this crap. All memory management is forced to take place as early as possible and failures are treated as kernel panics.

    // ==== MAIN INITIALIZATION ====

    // Initialize PC Screen Font for later
    psfInit();

    // Initialize serial logging
    serialInit();
    serialPrintf("============================================================================================================================\n");
    serialPrintf("\treduceOS v%s %s - written by sasdallas\n", __kernel_version, __kernel_codename);
    serialPrintf("\tBuild %s-%s, compiled on %s\n", __kernel_build_number, __kernel_configuration, __kernel_build_date);
    serialPrintf("============================================================================================================================\n\n");
    serialPrintf("Kernel location: 0x%x - 0x%x\nText section: 0x%x - 0x%x; Data section: 0x%x - 0x%x; BSS section: 0x%x - 0x%x\n",
                 &text_start, &bss_end, &text_start, &text_end, &data_start, &data_end, &bss_start, &bss_end);
    serialPrintf("Loader magic: 0x%x\n\n", loader_magic);
    serialPrintf("Serial logging initialized!\n");

    

    if (loader_magic != 0x43D8C305) {
        serialPrintf("loader magic: 0x%x addr: 0x%x\n", loader_magic, addr);
        panic("kernel", "kmain", "loader_magic != 0x43D8C305");
    }

    // Before CPU initialization can even take place, we should start the clock.
    clock_init();

    // Now initialize ACPI
    // acpiInit();

    // Initialize the hardware abstraction layer
    hal_init();
    
    // While we're doing completely unrelated stuff, setup the argument parser
    args_init((char*)globalInfo->m_cmdLine);

    // Installs the GDT and IDT entries for BIOS32
    bios32_init();
    serialPrintf("kernel: bios32 initialized successfully!\n");


    // ==== TERMINAL INITIALIZATION ====

    // Quickbooting allows us to skip all terminal initialization until the system is ready to go and loaded. If --quickboot was specified, omit anything terminal-related.
    // Setting up video drivers is still required, though.
    video_init();


    if (args_has("--quickboot")) goto _skip_terminal;
    initTerminal();

    updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color
    terminalUpdateTopBarKernel("created by @sasdallas");

    // Next, update terminal color to the proper color and print loading text.
    updateTerminalColor_gfx(COLOR_WHITE, COLOR_CYAN);

    printf("reduceOS is loading, please wait...\n");


    // ==== PERIPHERAL/DRIVER INITIALIZATION ====

_skip_terminal:
    // PIT timer
    updateBottomText("Initializing PIT...");
    pitInit();

    // Initialize keyboard
    updateBottomText("Initializing keyboard...");
    keyboardInitialize();
    setKBHandler(true);
    serialPrintf("kernel: Keyboard handler initialized.\n");

    // Setup the DMA pool
    // I can hear the page fault ready to happen because DMA wasn't properly allocated.
    dma_initPool(256 * 1024); // 256 KB of DMA pool

    // Initialize the floppy drive (will do IRQ waiting loop occasionally - NEED to fix)
    floppy_init();
    serialPrintf("kernel: Initialized floppy drive successfully.\n");

    // Probe for PCI devices
    updateBottomText("Probing PCI...");
    initPCI();
    serialPrintf("kernel: PCI probe completed\n");

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
    keyboard_devinit();     // /device/keyboard

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
            // There should be a way to dynamically do this.
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

    // Initial ramdisk contains kernel symbol map (essential for module loads)
    // Perhaps there could be an option to disable this?
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

    printf("Mounted nodes successfully.\n");

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

    // Scan and initialize modules for kernelspace 
    printf("Starting up modules...\n");
    if (!args_has("--no_modules")) module_parseCFG();

    printf("Kernel loading completed.\n");
    useCommands();
}

void useCommands() {
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

    registerCommand("mount_fat", (command*)mountFAT);

    registerCommand("ls", (command*)ls);
    registerCommand("cd", (command*)cd);
    registerCommand("cat", (command*)cat);
    registerCommand("create", (command*)create);
    registerCommand("mkdir", (command*)mkdir);
    registerCommand("pwd", (command*)pwd);
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
    registerCommand("leak", (command*)leak_memory);
    registerCommand("time", (command*)gtime);
    registerCommand("drun", (command*)drun);

    serialPrintf("kmain: All commands registered successfully.\n");

    // Finalize the memory map
    mem_finalize();

    if (args_has("--no_tasking")) goto _use_commands_done; // if you use this, I am sorry
    
    // Startup tasking, signals, and TTY
    tasking_start();
    signal_init();
    tty_init();


_use_commands_done: // amazing label name

    // yawn.. where were we again?
    if (args_has("--quickboot")) {
        initTerminal();

        updateTerminalColor_gfx(COLOR_BLACK, COLOR_LIGHT_GRAY); // Update terminal color
        terminalUpdateTopBarKernel("created by @sasdallas");

        // Next, update terminal color to the proper color and print loading text.
        updateTerminalColor_gfx(COLOR_WHITE, COLOR_CYAN);

        printf("reduceOS is loading, please wait...\n");
        printf("WARNING: Quickbooted (terminal omitted)\n");
    }

    // Calculate the boottime
    struct timeval tv;
    gettimeofday(&tv, NULL);

    serialPrintf("kernel: boot sequence completed - reduceOS has loaded successfully\n");
    serialPrintf("\tboot completed in %i seconds\n", tv.tv_sec - clock_getBoottime());
    kernel_boot_time = tv.tv_sec - clock_getBoottime();

    printf("reduceOS has finished loading successfully.\n");
    printf("Please type your commands below.\n");
    printf("Type 'help' for help.\n");
    if (args_has("--force_vga"))
        printf("WARNING: You are currently in VGA text mode. This mode is deprecated and unsupported!\n");
    if (!strcmp(fs_root->name, "tarfs"))
        printf("WARNING: No root filesystem was mounted. The initial ramdisk has been mounted as root.\n");


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
