// test.c - Test command for reduceOS

#include "include/test.h"

#include "include/libc/stdint.h" 
#include "include/libc/string.h"

#include "include/fat.h"
#include "include/serial.h"
#include "include/terminal.h"
#include "include/floppy.h"
#include "include/ext2.h"


extern ideDevice_t ideDevices[4];
extern fsNode_t *fatDriver;

tree_comparator_t test_comparator(void *a, void *b) {
       return (a == b);
}

void test_debug_print_tree_node(tree_t *tree, tree_node_t *node, size_t height) {
    if (!node) return;

    // Indent output according to height
    char indents[18];
    memset(indents, 0, 18);
    for (int i = 0; i < height; i++) indents[i] = ' ';


    // Get the current node
    // Print it out
    printf("\t%s0x%x\n", indents, node->value);

    foreach(child, node->children) {
        test_debug_print_tree_node(tree, child->value, height + 1);
    }
}

int pmm_tests() {
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

    return 0;
}

int liballoc_tests() {
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
        
    printf("\tAllocating a large amount of memory...");

    uint8_t *memoryArray[2000];
    for (int i = 0; i < 2000; i++) {
        memoryArray[i] = kmalloc(8);
    }

    printf("PASS\n");

    printf("\tFreeing a large amount of memory...");
    for (int i = 0; i < 2000; i++) {
        kfree(memoryArray[i]);
    }

    printf("PASS\n");

    printf("\tAllocating 120KB...");
    uint8_t *itsgonnacrash = kmalloc(120000);
    printf("PASS\n");

    printf("\tAllocating 4KB five times...");
    uint8_t *one = kmalloc(4096);
    uint8_t *two = kmalloc(4096);
    uint8_t *three = kmalloc(4096);
    uint8_t *four = kmalloc(4096);
    uint8_t *five = kmalloc(4096);
    printf("PASS\n");

    printf("\tFreeing 4KB five times...");
    kfree(one);
    kfree(two);
    kfree(three);
    kfree(four);
    kfree(five);
    printf("PASS\n");

    printf("\tFreeing 120KB...");
    kfree(itsgonnacrash); // It did not in fact.
    printf("PASS\n");


    return 0;
}

int bios32_tests() {
    printf("\tServing INT 0x15...\n");
    REGISTERS_16 in, out = {0};

    in.ax = 0xE820;
    bios32_call(0x15, &in, &out);

    printf("\tInterrupt serviced. Results:\n");
    printf("\tAX = 0x%x BX = 0x%x CX = 0x%x DX = 0x%x\n", out.ax, out.bx, out.cx, out.dx);

    return 0;
}

int floppy_tests() {
    uint32_t sector = 0;
    uint8_t *buffer = 0;

    printf("\tReading sector %i...\n", sector);

    int ret = floppy_readSector(sector, &buffer);
    
    if (ret != FLOPPY_OK) {
        printf("Could not read sector. Error code %i\n", ret);
        return -1;
    }

    printf("\tContents of sector %i:\n", sector);

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

int ide_tests() {
    if (1) {
        // Because of the original way test() was constructed, there's an extra tab before the actual code on every line.
        // I am lazy and I don't want to remove it. So we do this.

        int fail = 0;

        int drive = -1;
        for (int i = 0; i < 4; i++) { 
            if (ideDevices[i].reserved == 1 && ideDevices[i].size > 1) {
                printf("\tFound IDE device with %i KB\n", ideDevices[i].size);
                drive = i;
                break;
            }
        }

        if (drive == -1) {
            printf("\tNo drives found or capacity too low to read sector.\n");
            return -1;
        }

        printf("\tTesting ideReadSectors (read at LBA 2)...");

        uint8_t *sector = kmalloc(512);
        ideReadSectors(drive, 1, 2, sector);
        printf("PASS (start 0x%x end 0x%x)\n", sector[0], sector[511]);

        printf("\tTesting ideWriteSectors (writing 0xFF)...");
        
        uint8_t *write_buffer = kmalloc(512);
        memset(write_buffer, 0xFF, 512);
        ideWriteSectors(drive, 1, 2, write_buffer);

        // Now, verify (reuse write_buffer to save memory)
        memset(write_buffer, 0x0, 512);
        ideReadSectors(drive, 1, 2, write_buffer);

        if (write_buffer[0] != 0xFF) {
            printf("FAIL (read 0x%x after writing instead of 0xFF)\n", write_buffer[0]);
            fail = 1;
        } else {
            printf("PASS (read 0x%x after writing)\n", write_buffer[0]);
        }

        printf("\tRestoring sector...");
        ideWriteSectors(drive, 1, 2, sector);
        printf("DONE\n");

        kfree(sector);
        kfree(write_buffer);

        printf("\tCreating VFS node...");
        fsNode_t *ret = kmalloc(sizeof(fsNode_t));

        ret->mask = ret->uid = ret->gid = ret->inode = ret->length = 0;
        ret->flags = VFS_DIRECTORY;
        ret->open = 0; // No lol
        ret->close = 0;
        ret->read = &ideRead_vfs;
        ret->write = &ideWrite_vfs;
        ret->readdir = 0;
        ret->finddir = 0;
        ret->impl = drive;
        ret->ptr = 0;

        printf("DONE\n");

        uint8_t *comparison_buffer = kmalloc(2048);
        ideReadSectors(drive, 4, 0, comparison_buffer);

        printf("\tTesting VFS node read (offset 0 size 1120)...");
        sector = kmalloc(1120);        
        int error = ret->read(ret, 0, 1120, sector);

        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i]); fail = 1; break; } 
            }

            if (match) printf("PASS\n");
        }

        memset(sector, 0, 1120);
        printf("\tTesting VFS node read (offset 512, size 1120)...");
        error = ret->read(ret, 512, 1120, sector);

        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i + 512]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i + 512]); fail = 1; break; } 
            }

            if (match) printf("PASS\n");
        }


        memset(sector, 0, 1120);
        printf("\tTesting VFS node read (offset 723, size 1120)...");
        
        error = ret->read(ret, 723, 1120, sector);

        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i + 723]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i + 723]); fail = 1; break;} 
            }

            if (match) printf("PASS\n");
        }

        kfree(sector);

        sector = kmalloc(600);
        uint8_t *sector_comparison = kmalloc(600); // This is for checking the sectors are actually correct.


        printf("\tTesting VFS node write (offset 0, size 600)...");
        memset(sector, 0xFF, 600);
        memset(sector_comparison, 0, 600);

        error = ret->write(ret, 0, 600, sector);
        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Read in and check the sector
            error = ret->read(ret, 0, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
                fail = 1;
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); fail = 1; break;}
                }

                if (match) printf("PASS\n");
            }
        }

        printf("\tTesting VFS node write (offset 80, size 600)...");
        memset(sector, 0xF8, 600); // Different value just in case
        memset(sector_comparison, 0, 600);

        error = ret->write(ret, 80, 600, sector);
        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Read in and check the sector
            error = ret->read(ret, 80, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
                fail = 1;
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); fail = 1; break;}
                }

                if (match) printf("PASS\n");
            }
        }

        printf("\tTesting VFS node write (offset 763, size 600)...");
        memset(sector, 0xFB, 600); // Different value just in case
        memset(sector_comparison, 0, 600);

        error = ret->write(ret, 763, 600, sector);
        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
            fail = 1;
        } else {
            // Read in and check the sector
            error = ret->read(ret, 763, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
                fail = 1;
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); fail = 1; break;}
                }

                if (match) printf("PASS\n");
            }
        }




        printf("\tRestoring sector...");
        ideWriteSectors(drive, 4, 0, comparison_buffer);
        printf("DONE\n");

        kfree(comparison_buffer);
        kfree(sector);
        kfree(sector_comparison);

        return fail;
    }
}


int fat_tests() {
    if (1) {

        int fail = 0;

        // See ide_tests()
        if (!fatDriver) {
            printf("\tFAT driver is not running\n");    
            return -1;
        }

        
        printf("\tTesting fatOpenInternal (test.txt)...");

        fsNode_t ret = fatOpenInternal(fatDriver, "/test.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }


        printf("\tTesting fatOpenInternal (dir/test.txt)...");

        ret = fatOpenInternal(fatDriver, "/dir/test.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        printf("\tTesting fatOpenInternal (nonexistent.txt)...");

        ret = fatOpenInternal(fatDriver, "/nonexistent.txt");
        if (ret.flags != -1) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        uint8_t *buffer;

        printf("\tTesting fatReadInternal (test.txt, 1 cluster)...");

        ret = fatOpenInternal(fatDriver, "/test.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
        } else {
            buffer = kmalloc(ret.length);
            if (fatReadInternal(&ret, buffer, ret.length) == EOF) {
                for (int j = 0; j < ret.length; j++) serialPrintf("%c", buffer[j]);
                printf("PASS\n");
            } else {
                printf("FAIL (file spans >1 cluster)\n");
                fail = 1;
            }
            kfree(buffer);
        }

        printf("\tTesting fatReadInternal (cluster.txt, 4 clusters)...");

        ret = fatOpenInternal(fatDriver, "/cluster.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(4 * 4 * 512);
            
            bool func_fail = false;
            int returnValue;
            for (int i = 0; i < 4; i++) {
                if (returnValue == EOF) { 
                    printf("FAIL (file spans <4 clusters)\n" );
                    func_fail = true;
                    fail = 1;
                    break;
                }
                returnValue = fatReadInternal(&ret, buffer + (i*4*512), ret.length); // Bugged bc we dont know sectors per cluster so we assume 4
            }

            if (returnValue != EOF) {
                printf("FAIL (file spans >4 clusters)\n");
            } else if (!func_fail) {
                printf("PASS\n");
            }

            kfree(buffer);
        }
    
        printf("\tReading in test.txt for next set of tests...");
        ret = fatOpenInternal(fatDriver, "/test.txt");
        uint8_t *comparison_buffer = kmalloc(ret.length);

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            fatReadInternal(&ret, comparison_buffer, ret.length);
            printf("DONE\n");
        }

        printf("\tTesting fatRead (test.txt, offset 0, size 100)...");
        

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(100);
            if (fatRead(&ret, 0, 100, buffer) != 0) {
                printf("FAIL (fatRead returned error)\n");
            } else {
                bool success = true;
                for (int i = 0; i < 100; i++) {
                    if (buffer[i] != comparison_buffer[i]) {
                        printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, buffer[i], comparison_buffer[i]);
                        success = false;
                        fail = 1;
                        break;
                    }
                }

                if (success) printf("PASS\n");
            }
            kfree(buffer);
        }

        printf("\tTesting fatRead (test.txt, offset 26, size 102)...");
        

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(102);
            if (fatRead(&ret, 26, 102, buffer) != 0) {
                printf("FAIL (fatRead returned error)\n");
            } else {
                bool success = true;
                for (int i = 0; i < 102; i++) {
                    if (buffer[i] != comparison_buffer[i + 26]) {
                        printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, buffer[i], comparison_buffer[i + 26]);
                        success = false;
                        fail = 1;
                        break;
                    }
                }

                if (success) printf("PASS\n");
            }
            kfree(buffer);
        }


        printf("\tTesting fatRead (test.txt, offset 50, size 33)...");
        

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(33);
            if (fatRead(&ret, 50, 33, buffer) != 0) {
                printf("FAIL (fatRead returned error)\n");
            } else {
                bool success = true;
                for (int i = 0; i < 33; i++) {
                    if (buffer[i] != comparison_buffer[i + 50]) {
                        printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, buffer[i], comparison_buffer[i + 50]);
                        success = false;
                        fail = 1;
                        break;
                    }
                }

                if (success) printf("PASS\n");
            }
            kfree(buffer);
        }

        // It's ugly but we have to do it because of the VFS being incomplete.
        printf("\tTesting fatOpen (test.txt)...");
        memset(&ret, 0, sizeof(fsNode_t));
        ret.flags = -1; // Preventing false-passes
        strcpy(ret.name, "/test.txt");
        ret.impl_struct = fatDriver->impl_struct;

        fatOpen(&ret);

        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        printf("\tTesting fatOpen (dir/test.txt)...");
        memset(&ret, 0, sizeof(fsNode_t));
        ret.flags = -1; // Preventing false-passes
        strcpy(ret.name, "/dir/test.txt");
        ret.impl_struct = fatDriver->impl_struct;

        fatOpen(&ret);

        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }


        printf("\tTesting fatOpen (nonexistent.txt)...");
        memset(&ret, 0, sizeof(fsNode_t));
        ret.flags = -1; // Preventing false-passes
        strcpy(ret.name, "/nonexistent.txt");
        ret.impl_struct = fatDriver->impl_struct;

        fatOpen(&ret);

        if (ret.flags != -1) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }



        kfree(comparison_buffer);
        // kfree(ret); - Glitches out??

        return fail;
    }
}

int vfs_tests() {
    // Test the virtual filesystem
    int fail = 0;

   
    /* Canonicalizing path testing */
    int canonicalize_fail = 0;
    printf("\tTesting VFS path canonicalization...");

    char *path = "some_random_directory/path/file.txt";
    char *cwd = "/device/ide0";
    char *output_path = vfs_canonicalizePath(cwd, path);
    if (strcmp(output_path, "/device/ide0/some_random_directory/path/file.txt")) {
        // Did not pass
        canonicalize_fail = 1;
        fail = 1;
    }


    kfree(output_path);

    path = "some_random_directory/path/../anotherfile.txt";
    cwd = "/device/ide0";
    output_path = vfs_canonicalizePath(cwd, path);
    if (strcmp(output_path, "/device/ide0/some_random_directory/anotherfile.txt")) {
        // Did not pass
        canonicalize_fail = 2;
        fail = 1;
    }

    kfree(output_path);


    path = "some_random_directory/../another_directory/./again/again.txt";
    cwd = "/device/ide0";
    output_path = vfs_canonicalizePath(cwd, path);
    if (strcmp(output_path, "/device/ide0/another_directory/again/again.txt")) {
        // Did not pass
        canonicalize_fail = 3;
        fail = 1;
    }
    
    kfree(output_path);

    if (canonicalize_fail) printf("FAIL (pass %i)\n", canonicalize_fail);
    else printf("PASS\n");
    
    return 0;
}

// This function serves as a function to test multiple modules of the OS.
int test(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32, floppy, ide, fat\n");
        return 0;
    } 

    if (!strcmp(args[1], "pmm")) {
        printf("=== TESTING PHYSICAL MEMORY MANAGEMENT ===\n");
        
        if (pmm_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "liballoc")) {
        printf("=== TESTING LIBALLOC ===\n");

        if (liballoc_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "bios32")) {
        printf("=== TESTING BIOS32 ===\n");

        if (bios32_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "floppy")) {
        printf("=== TESTING FLOPPY ===\n");
        
        if (floppy_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "ide")) {
        printf("=== TESTING IDE ===\n");

        if (ide_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "fat")) {
        printf("=== TESTING FAT DRIVER ===\n"); 

        if (fat_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");

    } else if (!strcmp(args[1], "ext2_alloc")) {
        /*
        serialPrintf("WARNING: Running emergency liballoc-is-screwed-but-it-also-might-be-ext2-idk-lol test.\n");
        printf("=== RUNNING CRASH TEST ===\n");
        
        printf("\tCreating IDE node...");
        fsNode_t *node = ideGetVFSNode(0);
        printf("DONE (0x%x)\n", node);

        printf("\tAllocating ext2 superblock...");
        ext2_superblock_t *superblock = kmalloc(sizeof(ext2_superblock_t));
        printf("DONE (0x%x)\n", superblock);

        printf("\tAllocating ext2_t type...");
        ext2_t *ext2_filesystem = kmalloc(sizeof(ext2_t));
        printf("DONE (0x%x)\n", ext2_filesystem);

        printf("\tAllocating BGD list...");
        ext2_filesystem->bgd_list = kmalloc(4096 * (15 * sizeof(ext2_bgd_t) / 4096 + 1)); // Average block size is 4096
        printf("DONE (0x%x)\n", ext2_filesystem->bgd_list);

        printf("\tAllocating bg_buffer...");
        char *bg_buffer = kmalloc(4096 * sizeof(char));
        printf("DONE (0x%x)\n", bg_buffer);

        printf("\tFreeing bg_buffer...");
        kfree(bg_buffer);
        printf("DONE\n");

        printf("\tAllocating root inode...");
        ext2_inode_t *rootInode = kmalloc(sizeof(ext2_inode_t));
        memset(rootInode, 0, sizeof(ext2_inode_t));
        printf("DONE (0x%x)\n", rootInode);

        printf("\tAllocating VFS node...");
        fsNode_t *ext2 = kmalloc(sizeof(fsNode_t));
        printf("DONE (0x%x)\n", ext2);

        printf("\tFreeing root inode...");
        kfree(rootInode);
        printf("DONE\n");
        *//*
        printf("\tInitializing EXT2 driver...");
        fsNode_t *node = ext2_init(ideGetVFSNode(0));
        printf("DONE (fsNode = 0x%x)\n", node);

        printf("\t^ STAGE 1 COMPLETED - ENTERING CRASH ZONE ^\n");

        printf("\tAllocating inode...");
        ext2_inode_t *inode = kmalloc(sizeof(ext2_inode_t));
        printf("DONE (0x%x)\n", inode);

        printf("\tAllocating block...");
        uint8_t *block = kmalloc(4096);
        printf("DONE (0x%x)\n", block);

        printf("\tAllocating dname (12 length)...");
        char *dname = kmalloc(sizeof(char) * 12);
        printf("DONE (0x%x)\n", dname);

        printf("\tFreeing dname...");
        kfree(dname);
        printf("DONE\n");

        printf("\tAllocating dirent...");
        ext2_dirent_t *dirent = kmalloc(sizeof(ext2_dirent_t)); // GUESSING SIZE BECAUSE LAZY
        printf("DONE (0x%x)\n", dirent);

        printf("\tFreeing block...");
        kfree(block);
        printf("DONE\n");

        printf("\tAllocating file node (THIS WILL CRASH)...");
        fsNode_t *node2 = kmalloc(sizeof(fsNode_t));
        printf("...?\n");

        printf("\tFreeing stuff...");
        kfree(inode);
        kfree(node2);
        kfree(dirent);
        printf("DONE\n");

        printf("\tSneaking suspicion we passed the crash test. Let's up the difficulty!\n");
        printf("\t^ STAGE 2 COMPLETED - ENTERING CRASH ZONE 2 ^\n"); */

        printf("\tSit back and relax, your computer is preparing to crash...");
        int memoryAllocated = 0;
        while (true) {
            // average C program:
            fsNode_t *node = kmalloc(sizeof(fsNode_t));
            memoryAllocated += sizeof(fsNode_t);

            printf("\r\tSit back and relax, your computer is preparing to crash... %i KB", memoryAllocated / 1024);
            
        }
    } else if (!strcmp(args[1], "tree")) {
        // to be moved
        printf("=== TESTING TREE ===\n"); 

        printf("\tTesting tree_create...");
        tree_t *tree = tree_create();
        if (tree) printf("PASS\n");
        else {
            printf("FAIL\n");
            printf("=== TESTS FAILED ===\n");
            return -1;
        }

        printf("\tTesting tree_set_root...");
        tree_set_root(tree, 0xB16B00B5);
        
        if (tree->root->value == 0xB16B00B5) printf("PASS\n");
        else printf("FAIL\n");

        printf("\tTesting tree_node_create...");
        
        tree_node_t *node = tree_node_create(0x11111111);
        if (node->value == 0x11111111) printf("PASS\n");
        else printf("FAIL\n");

        printf("\tTesting tree_insert_child_node (root/node)...");
        tree_node_insert_child_node(tree, tree->root, node);
        printf("PASS\n");

        printf("\tTesting tree_find...");

        tree_node_t *returned_node = tree_find(tree, 0x11111111, test_comparator);
        if (returned_node->value == 0x11111111) printf("PASS\n");
        else printf("FAIL\n");

        printf("\tTesting tree_node_remove (0 children)...");
        tree_node_remove(tree, returned_node);
    
        if (tree_find(tree, 0x11111111, test_comparator) == NULL) printf("PASS\n");
        else printf("FAIL\n");

        printf("\tTesting tree_node_insert_child...");
        tree_node_insert_child(tree, tree->root, 0x1);
        if (tree_count_children(tree->root) == 1) printf("PASS\n");
        else printf("FAIL\n");

        // We already tested tree_count_children, technically (even though it might be wrong) ;)
        printf("\tTesting tree_count_children...PASS\n");

        kfree(node);


        printf("\tFilling tree with data...");
        for (int i = 0; i < 3; i++) {
            tree_node_t *n = tree_node_insert_child(tree, tree->root, i * 4); // i * 4 is just a random value from my head
            for (int j = 0; j < 4; j++) {
                tree_node_insert_child(tree, n, i*4 + j + 1);
            }
        }
        printf("DONE\n");

        
        printf("\tTesting tree_node_find_parent...");
        
        // We'll test this function on a specific tree node
        tree_node_t *test_node = tree_find(tree, 0xC, test_comparator);

        if (test_node) {
            tree_node_t *parent = tree_find_parent(tree, test_node);
            if (parent) {
                if (parent->value == 0x8) printf("PASS\n");
                else printf("FAIL (parent->value = 0x%x)\n", parent->value);
            } else {
                printf("FAIL (returned NULL)\n");
            }
        } else {
            printf("FAIL (tree_find failed)\n");
        }

        
        printf("\tDestroying tree...");
        tree_free(tree);
        printf("DONE\n");

        printf("=== TESTS COMPLETED ===\n");

    } else if (!strcmp(args[1], "vfs")) {
        printf("=== TESTING VFS ===\n");

        if (vfs_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32, floppy, ide, fat, tree, vfs\n");
    }

    return 0;
}

