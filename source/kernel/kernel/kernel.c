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


void usermodeMain() {

}




fsNode_t *fatDriver = NULL;
fsNode_t *ext2_root = NULL;

int ls(int argc, char *args[]) {
    if (argc > 2) {
        printf("Usage: ls <directory (optional)>\n");
        return -1;
    }

    if (!fs_root) {
        printf("No filesystem is currently mounted.\n");
        return -1;
    }

    char *dir = kmalloc(256);
    strcpy(dir, get_cwd());
    if (argc == 2) strcpy(dir, args[1]);

    // Open the directory
    fsNode_t *directory = open_file(dir, 0);
    


    // Sorry if this is a little messy - I had a REALLY bad time debugging a specific function that wasn't even related. Too burnt out to clean up.

    if (directory && directory->flags == VFS_DIRECTORY) {
        // TODO: EXT2 and FAT driver use custom names to identify themselves when returned.
        // This is cool and all, but it makes it weird when it says "Files in directory 'EXT2 driver':", so we'll use dir instead.
        printf("Files in directory '%s':\n", dir);

        uint32_t i = 0;
        while (true) {
            struct dirent *direntry = directory->readdir(directory, i);
            if (!direntry || !direntry->name) break;  

            printf("%s\n", direntry->name);
            i++;
            kfree(direntry);
        }

    } else {
        printf("Directory '%s' not found\n", dir);
    }

    kfree(dir);

    return 0;
}  

int cd(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: cd <directory>\n");
        return -1;
    }

    fsNode_t *file = open_file(args[1], 0);
    if (!file) {
        printf("Directory '%s' not found\n", args[1]);
        return -1;
    }

    if (file->flags != VFS_DIRECTORY) {
        printf("'%s' is not a directory\n", args[1]);
        kfree(file);
        return -1;
    }

    kfree(file);

    change_cwd(args[1]);
    return 0;
}

int cat(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: cat <file>\n");
        return -1;
    }

    // We just need to open the file, read its contents and then print them.
    fsNode_t *file = open_file(args[1], 0);

    if (!file) {
        printf("File '%s' not found\n", args[1]);
        return -1;
    }

    if (file->flags != VFS_FILE) {
        printf("'%s' is not a file\n", args[1]);
        kfree(file);
        return -1;
    }

    if (file->length == 0) {
        printf("File '%s' is empty\n", args[1]);
        kfree(file);
        return -1;
    }

    uint8_t *buffer = kmalloc(file->length);
    int ret = file->read(file, 0, file->length, buffer);

    if (ret != file->length) {
        printf("Failed to read the file (file->read returned %i).\n", ret);
        kfree(buffer);
        kfree(file);
        return -1;
    }

    printf("%s", buffer);

    kfree(buffer);
    kfree(file);

    return 0;
}

int mkdir(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: mkdir <directory>\n");
        return -1;
    }

    if (!fs_root) {
        printf("No filesystem is currently mounted.\n");
        return -1;
    }


    char *dirname = args[1];
    
    // these users will probably completely break everything ive ever loved in this code (there's not much) but im too tired to fix them
    // plus its funny

    // First, we have to canonicalize the path.
    char *path = vfs_canonicalizePath(get_cwd(), dirname);
    
    // Sadly, this will be complicated, but we can press on.
    // We need to chop off the last part of the path. As simple as this seems, it's not very simple.

    // The first thing we need to do is to verify that path doesn't end in a '/', and chop it off if it does
    if (path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    // Now, we iterate through the slashes until there are no more
    char *path2 = kmalloc(strlen(path));
    char *directory_name = kmalloc(100); 


    // Disgusting but works lol
    int slashes = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/') slashes++;
    }

    if (slashes == 1) {
        // Signifies root directory, set path2 to '/' and goto making_directory
        memset(path2, 0, strlen(path2));
        strcpy(path2, "/");
        strcpy(directory_name, dirname);
        goto making_directory;
    }

    int currentSlashes = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/') currentSlashes++;

        if (currentSlashes == slashes) {
            memset(directory_name, 0, 100);
            memcpy(directory_name, path + i + 1, strlen(path) - i - 1);
            break;
        }

        path2[i] = path[i];
    }


making_directory:
    printf("Creating directory '%s' (path: '%s')...\n", directory_name, path2);

    fsNode_t *dir = open_file(path2, 0);
    if (dir) {
        dir->mkdir(dir, directory_name, 0);
        printf("Created directory successfully.\n");
    } else {
        printf("Path '%s' was not found.\n", path2);
    }
}

int create(int argc, char *args[]) {
    // Don't tell anyone but I literally just copy pasted mkdir

    if (argc != 2) {
        printf("Usage: create <filename>\n");
        return -1;
    }

    if (!fs_root) {
        printf("No filesystem is currently mounted.\n");
        return -1;
    }

    char *fname = args[1];
    
    // these users will probably completely break everything ive ever loved in this code (there's not much) but im too tired to fix them
    // plus its funny

    // First, we have to canonicalize the path.
    char *path = vfs_canonicalizePath(get_cwd(), fname);
    
    // Sadly, this will be complicated, but we can press on.
    // We need to chop off the last part of the path. As simple as this seems, it's not very simple.

    // The first thing we need to do is to verify that path doesn't end in a '/', and chop it off if it does
    if (path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    // Now, we iterate through the slashes until there are no more
    char *path2 = kmalloc(strlen(path));
    char *file_name = kmalloc(100); 


    // Disgusting but works lol
    int slashes = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/') slashes++;
    }

    if (slashes == 1) {
        // Signifies root directory, set path2 to '/' and goto making_file
        memset(path2, 0, strlen(path2));
        strcpy(path2, "/");
        strcpy(file_name, fname);
        goto making_file;
    }

    int currentSlashes = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/') currentSlashes++;

        if (currentSlashes == slashes) {
            memset(file_name, 0, 100);
            memcpy(file_name, path + i + 1, strlen(path) - i - 1);
            break;
        }

        path2[i] = path[i];
    }


making_file:
    printf("Creating file '%s' (path: '%s')...\n", file_name, path2);
    fsNode_t *dir = open_file(path2, 0);
    if (dir) {
        dir->create(dir, file_name, 0);
        printf("Created file successfully.\n");
    } else {
        printf("Path '%s' was not found.\n", path2);
    }

    return 0;
}

int pwd(int argc, char *args[]) {
    printf("%s\n", get_cwd());
    return 0;
}

int show_bitmap(int argc, char *args[]) {
    // This command will just show a bitmap the user requests.
    if (argc != 2) {
        printf("Usage: bitmap <filename>\n");
        return -1;
    }

    printf("Loading bitmap '%s'...\n", args[1]);
    fsNode_t *bitmap_file = open_file(args[1], 0);
    if (bitmap_file) {
        bitmap_t *bmp = bitmap_loadBitmap(bitmap_file);
        displayBitmap(bmp, 0, 0);
        kfree(bmp->imageBytes);
        kfree(bmp);
    } else {
        printf("File not found\n");
    }

    return 0;
}



int edit(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: edit <filename>\n");
        return -1;
    }

    // Load the file
    fsNode_t *file = open_file(args[1], 0);
    if (!file) {
        printf("File '%s' not found.\n", args[1]);
        return -1;
    }

    if (!file->write) {
        printf("File is not writable\n");
        return -1;
    }

    char *buffer = kmalloc(4096); // 4096 bytes to start off with
    printf("Welcome to the editor. Press ENTER + CTRL to exit.\n");

    int chars = 0;
    int currentlyAllocated = 4096;
    while (true) {
        char *tmp_buffer = kmalloc(4095);
        keyboardGetLine(tmp_buffer);
        strcpy(buffer + chars, tmp_buffer);
        chars += strlen(tmp_buffer);
        
        if (getControl()) break;

        buffer[chars] = '\n';
        chars++;
        kfree(tmp_buffer);
        


        if (chars+1 >= currentlyAllocated) {
            buffer = krealloc(buffer, currentlyAllocated * 2);
            currentlyAllocated = currentlyAllocated * 2;
        }
    }

    printf("\nDo you want to save your changes? [y/n] ");

    char *line = kmalloc(256); // Safety lol
    keyboardGetLine(line);

    if (!strcmp(line, "y") || !strcmp(line, "Y")) {
        printf("Saving, please wait..\n");
        int ret = file->write(file, 0, chars, buffer);
        if (ret != chars) {
            printf("Error: Write method returned %i.\n", ret);
        } else {
            printf("Saved successfully.\n");
        }
    } 

    kfree(line);
    kfree(buffer);
    return 0;

}

int mountFAT(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: mount_fat <directory, ex. /dev/ide>\n");
        return -1;
    }

    printf("Mounting %s to /dev/fat...\n", args[1]);
    int ret = vfs_mountType("fat", args[1], "/dev/fat");
    if (ret == 0) {
        printf("Successfully mounted to /dev/fat.\n");
        //change_cwd("/dev/");
        fatDriver = open_file("/dev/fat", 0);
        change_cwd("/");
    } else {
        printf("Could not mount the drive. Error code %i\n", ret);
    }

    return 0;
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

    // Before anything else is allocated, we need to deinitialize a few other regions.
    // Here is the list:
    /*
        - ACPI (0x000E0000 - 0x000FFFFF)
    */

   pmm_deinitRegion(0x000E0000, 0x000FFFFF-0x000E0000);

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


    // Here's a nasty little hack to get around liballoc messing everything up.
    // We force our secondary framebuffer to 0x85A000 - 0xB5A000
    
    // DEFINITELY sketchy! What's going on with this system? 
    serialPrintf("WARNING: Enabling liballoc! Stand away from the flames!\n");
    enable_liballoc();
    

    // Initialize the IDE controller.
    updateBottomText("Initializing IDE controller...");
    ideInit(0x1F0, 0x3F6, 0x170, 0x376, 0x000); // Initialize parallel IDE
    
    //ide_init();


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


    

    // Start paging if VBE was not initialized.
    useCommands();
}



void useCommands() {
    serialPrintf("kmain: Memory management online with %i KB of physical memory\n", pmm_getPhysicalMemorySize());


    clearScreen(COLOR_WHITE, COLOR_CYAN);
    
    clearBuffer();
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