// ==========================================================
// cmds.c - Holds the commands for the command line shell
// ==========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <stdio.h>
#include <kernel/cmds.h>
#include <kernel/kernel.h>
#include <kernel/vfs.h>
#include <kernel/ksym.h>
#include <kernel/pmm.h>
#include <kernel/elf.h>
#include <kernel/mod.h>
#include <kernel/module.h>
#include <kernel/process.h>

extern fsNode_t *fatDriver;
extern fsNode_t *ext2_root;
extern multiboot_info *globalInfo;
extern ideDevice_t ideDevices[4];



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

    fsNode_t *n = open_file((strcmp(fs_root->name, "initrd") ? "/device/initrd" : "/"), 0);

    while ((node = readDirectoryFilesystem(n, i)) != 0)
    {
        printf("Found file %s", node->name);
        fsNode_t *fsNode = findDirectoryFilesystem(n, node->name);

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




/* FILESYSTEM COMMANDS */



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

    printf("%s\n", buffer);

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

    if (!args[1]) {
        printf("You need to actually provide something.\n");
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
    printf("Writing to device/file: %s\n", args[1]);

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
        printf("Usage: mount_fat <directory, ex. /device/ide>\n");
        return -1;
    }

    printf("Mounting %s to /device/fat...\n", args[1]);
    int ret = vfs_mountType("fat", args[1], "/device/fat");
    if (ret == 0) {
        printf("Successfully mounted to /device/fat.\n");
        //change_cwd("/device/");
        fatDriver = open_file("/device/fat", 0);
        change_cwd("/");
    } else {
        printf("Could not mount the drive. Error code %i\n", ret);
    }

    return 0;
}


int rm(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: rm <file>\n");
        return -1;
    }

    unlinkFilesystem(args[1]);
}

int strace(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: strace <max frames>\n");
        return -1;
    }

    int maxFrames = atoi(args[1]);
    printf("Traceback for %i frames:\n", maxFrames);
    serialPrintf("Traceback for %i frames:\n", maxFrames);

    stack_frame *stk;
    asm volatile ("movl %%ebp, %0" : "=r"(stk));

    for (uint32_t frame = 0; stk && frame < maxFrames; frame++) {
        // Try to get the symbol
        ksym_symbol_t *sym = kmalloc(sizeof(ksym_symbol_t));
        int err = ksym_find_best_symbol(stk->eip, sym);
        if (err == -1) {
            printf("Frame %i: 0x%x (ksym did not initialize)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (called before alloc init/ksym_init)\n", frame, stk->eip);
        } else if (err == -2) {
            printf("Frame %i: 0x%x (no debug symbols loaded)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (no debug symbols loaded)\n", frame, stk->eip);
        } else if (err == 0) {
            printf("Frame %i: 0x%x (%s+0x%x)\n", frame, stk->eip, sym->symname, stk->eip - (long)sym->address);
            serialPrintf("FRAME %i: IP 0x%x (%s+0x%x - base func addr 0x%x)\n", frame, stk->eip, sym->symname, stk->eip - (long)sym->address, (long)sym->address);
        } else {
            printf("Frame %i: 0x%x (unknown error when getting symbols)\n", frame, stk->eip);
            serialPrintf("FRAME %i: IP 0x%x (err = %i, unknown)\n", frame, stk->eip, err);
        }

        kfree(sym);

        stk = stk->ebp;
    }
}

int pmm(int argc, char *args[]) {
    printf("Physical memory management statistics:\n");
    printf("\tMemory size: 0x%x / %i KB\n", pmm_getPhysicalMemorySize(), pmm_getPhysicalMemorySize());
    printf("\tUsed blocks: %i blocks\n", pmm_getUsedBlocks());
    printf("\tFree blocks: %i blocks\n", pmm_getFreeBlocks());
    printf("\n");
    pmm_printMemoryMap(globalInfo);
    return 0;
}

int vfs(int argc, char *args[]) {
    printf("VFS TREE DUMP:\n");
    debug_print_vfs_tree(true);
    return 0;
}

typedef int entry_func(int argc, char *args[]);

int makeProcess(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: start_process <ELF file>\n");
        return -1;
    }

    printf("Loading ELF '%s'...\n", args[1]);

    char *path = vfs_canonicalizePath(get_cwd(), args[1]);

    // Fork the kernel's process
    pid_t pid = fork();

    if (pid) {
        // Let's setup the stack now
        currentProcess->thread.page_directory = cloneKernelSpace2(vmm_getKernelDirectory());
        currentProcess->thread.refcount = 1;
        currentProcess->thread.pd_lock = spinlock_init();
        vmm_switchDirectory(currentProcess->thread.page_directory);
        serialPrintf("Success\n");
        int ret = createProcess(path);
    }  else {
        printf("No PID was given - waiting.\n");
        int status;
        waitpid(pid, &status, 0);
        printf("Exit status %i\n", status);
    }
    
}

int loadELF(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: load_elf <ELF file>\n");
        return -1;
    }

    printf("Loading ELF '%s'...\n", args[1]);

    char *path = vfs_canonicalizePath(get_cwd(), args[1]);

    fsNode_t *elf_file = open_file(path, 0);

    if (!elf_file) {
        printf("Error: File '%s' not found\n", args[1]);
        return -1;
    }

    if (elf_file->flags != VFS_FILE) {
        printf("Error: '%s' is not a file\n", args[1]);
        return -1;
    }


    char *fbuf = kmalloc(elf_file->length);
    int ret = elf_file->read(elf_file, 0, elf_file->length, fbuf);

    if (ret != elf_file->length) {
        kfree(path);
        kfree(fbuf);
        printf("Error: Failed to read the file '%s'.\n", args[1]);
        return -1;
    }
    void *addr = elf_loadFileFromBuffer(fbuf);

    if (addr == NULL) {
        printf("Error: Failed to load ELF file (check debug)\n");
    } else {
        printf("Successfully loaded ELF file at 0x%x\n", addr);
    }

    if (addr != 0x0 && addr != NULL) {
        entry_func *f = (entry_func*)addr;
        int ret = f(1, NULL);
        printf("Got %i from ELF file entry\n", ret);
    }

    elf_cleanupFile(fbuf);

    return 0;
}

int mountTAR(int argc, char *args[]) {
    if (argc != 3) {
        printf("Usage: mount_tar <filename> <mountpoint>\n");
        return -1;
    }

    char *mountpoint = vfs_canonicalizePath(get_cwd(), args[2]);
    char *filename = vfs_canonicalizePath(get_cwd(), args[1]);

    fsNode_t *file = open_file(filename, 0);
    
    if (!file) {
        kfree(mountpoint);
        kfree(filename);
        printf("Failed to open file '%s'\n", args[1]);
        return -1;
    }

    printf("Mounting '%s' to '%s'...\n", args[1], args[2]);
    // user does this wrong its THEIR fault not mine (even though I should've accounted for it)
    int ret = vfs_mountType("tar", file, mountpoint);

    if (ret == 0) {
        printf("Successfully mounted tar archive at %s.\n", mountpoint);
    } else {
        printf("Failed to mount to %s (ret = %i)\n", mountpoint, ret);
    }

    return 0;

    
}

int loadModule(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: modload <module file>\n");
        return -1;
    }

    printf("Loading module '%s'...\n", args[1]);

    char *path = vfs_canonicalizePath(get_cwd(), args[1]);
    fsNode_t *elf_file = open_file(path, 0);

    if (!elf_file) {
        printf("Error: File '%s' not found\n", args[1]);
        return -1;
    }

    if (elf_file->flags != VFS_FILE) {
        printf("Error: '%s' is not a file", args[1]);
        return -1;
    }

    struct Metadata *out = kmalloc(sizeof(struct Metadata));
    int ret = module_load(elf_file, 1, NULL, out);

    switch (ret) {
        case MODULE_OK: 
            printf("Successfully loaded module '%s'\n", out->name);
            break;
        case MODULE_LOAD_ERROR:
            printf("Failed to load module (ELF load fail)\n");
            break;
        case MODULE_CONF_ERROR:
            printf("Failed to load module (configuration error??)\n");
            break;
        case MODULE_META_ERROR:
            printf("Failed to load module (no metadata)\n");
            break;
        case MODULE_PARAM_ERROR:
            printf("Failed to load module (invalid parameters)\n");
            break;
        case MODULE_READ_ERROR:
            printf("Failed to load module (read error)\n");
            break;
        case MODULE_INIT_ERROR:
            printf("Failed to initialize module\n");
            break;
        default:
            printf("Unknown module error\n");
            break;
    }

    return 0;
}
