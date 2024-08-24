// test.c - Test command for reduceOS

#include <kernel/test.h>

#include <stdint.h> 
#include <string.h>

#include <kernel/fat.h>
#include <kernel/serial.h>
#include <kernel/terminal.h>
#include <kernel/floppy.h>
#include <kernel/ext2.h>
#include <kernel/bitmap.h>
#include <kernel/vfs.h>
#include <kernel/processor.h>


extern ideDevice_t ideDevices[4];
extern fsNode_t *fatDriver;
extern fsNode_t *ext2_root;



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


        // Calculate an LBA that's about 75% of the drive's capacity
        uint64_t capacity = ideGetDriveCapacity(drive);
        serialPrintf("Drive sectors: 0x%x\n", capacity);
        if (capacity <= 100) { printf("\tCannot perform the far read test.\n"); return -1; }
        int lba = capacity - 100;

        printf("\tPerforming far read test (LBA = 0x%x)...", lba);

        uint8_t *buffer = kmalloc(512);
        memset(buffer, 0xAA, 512);
        ideReadSectors(drive, 1, lba, buffer);
        for (int i = 0; i < 512; i++) serialPrintf("%x", buffer[i]);



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

        fsNode_t *ret; 

        ret = fatOpenInternal(fatDriver, "/test.txt");
        if (ret->flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }
        
        kfree(ret);



        printf("\tTesting fatOpenInternal (dir/test.txt)...");

        fsNode_t *ret2 = fatOpenInternal(fatDriver, "/dir/test.txt");
        if (ret2->flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret2->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }
        
        kfree(ret2);


        printf("\tTesting fatOpenInternal (nonexistent.txt)...");

        ret = fatOpenInternal(fatDriver, "/nonexistent.txt");
        if (ret->flags != -1) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        kfree(ret);


        printf("\tTesting fatOpenInternal (dir/)...");

        ret = fatOpenInternal(fatDriver, "/dir");
        if (ret->flags != VFS_DIRECTORY) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        kfree(ret);

        printf("\tTesting fatOpenInternal (dir/nested)...");

        ret = fatOpenInternal(fatDriver, "/dir/nested");
        if (ret->flags != VFS_DIRECTORY) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        kfree(ret);


        uint8_t *buffer;

        printf("\tTesting fatReadInternal (test.txt, 1 cluster)...");

        ret = fatOpenInternal(fatDriver, "/test.txt");
        if (ret->flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
        } else {
            buffer = kmalloc(ret->length);
            if (fatReadInternal(ret, buffer, ret->length) == EOF) {
                for (int j = 0; j < ret->length; j++) serialPrintf("%c", buffer[j]);
                printf("PASS\n");
            } else {
                printf("FAIL (file spans >1 cluster)\n");
                fail = 1;
            }
            kfree(buffer);
        }

        printf("\tTesting fatReadInternal (cluster.txt, 4 clusters)...");

        ret = fatOpenInternal(fatDriver, "/cluster.txt");
        if (ret->flags != VFS_FILE) {
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
                returnValue = fatReadInternal(ret, buffer + (i*4*512), ret->length); // Bugged bc we dont know sectors per cluster so we assume 4
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
        uint8_t *comparison_buffer = kmalloc(ret->length);

        if (ret->flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            fatReadInternal(ret, comparison_buffer, ret->length);
            printf("DONE\n");
        }

        printf("\tTesting fatRead (test.txt, offset 0, size 100)...");
        

        if (ret->flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(100);
            if (fatRead(ret, 0, 100, buffer) != 0) {
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
        

        if (ret->flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(102);
            if (fatRead(ret, 26, 102, buffer) != 0) {
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
        

        if (ret->flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
            fail = 1;
        } else {
            buffer = kmalloc(33);
            if (fatRead(ret, 50, 33, buffer) != 0) {
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
        memset(ret, 0, sizeof(fsNode_t));
        ret->flags = -1; // Preventing false-passes
        strcpy(ret->name, "/test.txt");
        ret->impl_struct = fatDriver->impl_struct;

        fatOpen(ret);

        if (ret->flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }

        printf("\tTesting fatOpen (dir/test.txt)...");
        memset(ret, 0, sizeof(fsNode_t));
        ret->flags = -1; // Preventing false-passes
        strcpy(ret->name, "/dir/test.txt");
        ret->impl_struct = fatDriver->impl_struct;

        fatOpen(ret);

        if (ret->flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }


        printf("\tTesting fatOpen (nonexistent.txt)...");
        memset(ret, 0, sizeof(fsNode_t));
        ret->flags = -1; // Preventing false-passes
        strcpy(ret->name, "/nonexistent.txt");
        ret->impl_struct = fatDriver->impl_struct;

        fatOpen(ret);

        if (ret->flags != -1) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }



        printf("\tTesting fatOpen (dir/)...");
        memset(ret, 0, sizeof(fsNode_t));
        ret->flags = -1; // Preventing false-passes
        strcpy(ret->name, "/dir/");
        ret->impl_struct = fatDriver->impl_struct;

        fatOpen(ret);

        if (ret->flags != VFS_DIRECTORY) {
            printf("FAIL (flags = 0x%x)\n", ret->flags);
            fail = 1;
        } else {
            printf("PASS\n");
        }


        // This is a bugcheck for the user passing fatDriver.
        printf("\tTesting fatFindDirectory (test.txt)...");
        ret2 = fatFindDirectory(fatDriver, "test.txt");
        if (!ret2) {
            printf("FAIL\n");
        } else {
            printf("PASS\n");
        }

        // This is a bugcheck for the user using open_file to get the root.
        printf("\tRunning open_file finddir bugcheck...");
        fsNode_t *root = open_file("/dev/fat", 0); // hardcoded path alert, call the police!
        if (root) {
            ret2 = fatFindDirectory(root, "test.txt");
            if (!ret2) {
                printf("FAIL\n");
            } else {
                printf("PASS\n");
            }
        } else {
            printf("FAIL (did you not mount to /dev/fat?)\n");
        }


        printf("\tTesting fatFindDirectory (dir/test.txt)...");
        ret2 = fatFindDirectory(ret, "test.txt");
        
        if (!ret2) {
            printf("FAIL\n");
        } else {
            printf("PASS\n");
        }

        printf("\tTesting fatFindDirectory (dir/nested)...");
        ret2 = fatFindDirectory(ret, "nested/");
        
        if (!ret2) {
            printf("FAIL\n");
        } else {
            printf("PASS\n");
            printf("\tTesting fatFindDirectory (dir/nested/nest.txt)...");
            fsNode_t *ret3 = fatFindDirectory(ret2, "nest.txt");
        
            if (!ret3) {
                printf("FAIL\n");
            } else {
                printf("PASS\n");
                kfree(ret3);
            }
            
            kfree(ret2);
        }


        printf("\tTesting fatFindDirectory (nonexistant.txt)...");

        ret = fatFindDirectory(fatDriver, "nonexistant.txt");
        if (!ret) {
            printf("PASS\n");
        } else {
            printf("FAIL\n");
            kfree(ret);
        }

        printf("\tPerforming final finddir bugcheck (/)...");

        ret = fatFindDirectory(fatDriver, "/");
        if (!ret) {
            printf("FAIL\n");
        } else {
            printf("PASS\n");
            kfree(ret);
        }

        
        kfree(comparison_buffer);

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
    

    // Now, let's test the open_file function, as well as CWD support
    change_cwd("/");

    
    // Open the file test.txt
    printf("\tTesting open_file (test.txt)...");
    fsNode_t *f = open_file("/test.txt", 0);

    if (f != NULL) {
        printf("PASS\n");
        kfree(f);
    } else printf("FAIL\n");


    // Make sure we're not being fooled


    return 0;
}

int ext2_tests() {
    if (ext2_root == NULL) {
        printf("\tEXT2 device was not found\n");
        return -1;
    }

    printf("\tTesting ext2_finddir...");

    int fail = -1;

    fsNode_t *test_file = ext2_root->finddir(ext2_root, "test.txt");
    if (!test_file) {
        // TECHNICALLY THIS COUNTS AS A TEST BUT I REALLY DONT CARE
        ext2_root->create(ext2_root, "test.txt", EXT2_PERM_OX);
        test_file = ext2_root->finddir(ext2_root, "test.txt");

        if (!test_file) {
            printf("FAIL (couldnt find test.txt, failed to create it)\n");
            return -1;
        }

        char *a = kmalloc(1024);
        strcpy(a, "Hello, ext2 world! This is a test file for reading in our driver\nThis file was automatically generated by the ext2 testing system. You may remove it if you wish.\n");
        test_file->write(test_file, 0, strlen(a), a);
        kfree(a);

        fail = 1;
    }

    fsNode_t *directory = ext2_root->finddir(ext2_root, "subdir");
    if (!directory) {
        fail = 2;
    } 

    if (directory) {
        fsNode_t *nested = ext2_root->finddir(directory, "subdir.txt");
        if (!nested) {
            fail = 3;
        } else {
            kfree(nested);
        }
    }

    fsNode_t *non_existant = ext2_root->finddir(ext2_root, "nonexistant.txt");
    if (non_existant) {
        fail = 4;
    }

    switch (fail) {
        case -1:
            printf("PASS\n");
            break;
        case 1:
            printf("FAIL (test.txt)\n");
            break;
        case 2:
            printf("FAIL (subdir)\n");
            break;
        case 3:
            printf("FAIL (subdir/subdir.txt)\n");
            printf("\tWARNING: This indicates a long read failure. To prevent damage to disks, the tests will be terminated.\n");
            return -1;
            break;
        case 4:
            printf("FAIL (nonexistant.txt)\n");
            break;
        default:
            printf("FAIL (?)\n");
    }

    if (fail == -1) {
        kfree(directory); // Free directory
    }

    // Reset fail, we'll use it later.
    fail = -1;


    printf("\tTesting ext2_read (test.txt, full file size)...");
    uint8_t *comparisonBuffer = kmalloc(test_file->length);
    int ret = test_file->read(test_file, 0, test_file->length, comparisonBuffer);
    if (ret == test_file->length) printf("PASS\n");
    else { printf("FAIL (ret = %i)\n", ret); fail = 1; }

    if (fail == -1) {
        uint8_t *buffer = kmalloc(test_file->length);
        
        printf("\tTesting ext2_read (test.txt, offset 0, size 100)...");
        int ret = test_file->read(test_file, 0, 100, buffer);

        if (ret == 100) {
            bool checkFail = false;
            for (int i = 0; i < 100; i++) {
                if (buffer[i] != comparisonBuffer[i]) { printf("FAIL (mismatch at index %i - %c vs %c)\n", i, buffer[i], comparisonBuffer[i]); checkFail = true; }
                if (checkFail) break;
            }

            if (!checkFail) printf("PASS\n");
        } else {
            printf("FAIL (ret = %i)\n", ret);
        }

        memset(buffer, 0, test_file->length);


        printf("\tTesting ext2_read (test.txt, offset 25, size 100)...");
        ret = test_file->read(test_file, 25, 100, buffer);

        if (ret == 100) {
            bool checkFail = false;
            for (int i = 0; i < 100; i++) {
                if (buffer[i] != comparisonBuffer[i + 25]) { printf("FAIL (mismatch at index %i - %c vs %c)\n", i, buffer[i], comparisonBuffer[i + 25]); checkFail = true; }
                if (checkFail) break;
            }

            if (!checkFail) printf("PASS\n");
        } else {
            printf("FAIL (ret = %i)\n", ret);
        }

        kfree(buffer);
    }


    printf("\tReading file contents for restoration...");
    uint8_t *restoreBuffer = kmalloc(test_file->length);
    
    ret = test_file->read(test_file, 0, test_file->length, restoreBuffer);
    if (ret == test_file->length) {
        printf("DONE\n");

        // Clear the comparison buffer
        memset(comparisonBuffer, 0, sizeof(comparisonBuffer));
        
        uint8_t *buffer = kmalloc(test_file->length);


        // Copy in the exclamation points, then copy the string reduceOS is cool
        memset(buffer, 0, test_file->length);
        memset(buffer, '!', test_file->length);
        strcpy(buffer, "reduceOS is cool");

        printf("\tTesting ext2_write (test.txt, offset 0, full file size)...");
        test_file->write(test_file, 0, test_file->length, buffer);
    
        // Read in the file to check it
        test_file->read(test_file, 0, test_file->length, comparisonBuffer);

        bool checkFail = false;
        for (int i = 0; i < test_file->length; i++) {
            if (comparisonBuffer[i] != buffer[i]) {
                printf("FAIL (mismatch at index %i - %c vs %c)\n", i, buffer[i], comparisonBuffer[i]);
                checkFail = true;
                break;
            }
        }

        if (!checkFail) printf("PASS\n");

        memset(buffer, 0, test_file->length);
        memset(buffer, 'A', 25);

        printf("\tTesting ext2_write (test.txt, offset 0, size 25)...");
        test_file->write(test_file, 0, 25, buffer);
    
        // Read in the file to check it
        test_file->read(test_file, 0, 25, comparisonBuffer);

        checkFail = false;
        for (int i = 0; i < 25; i++) {
            if (comparisonBuffer[i] != buffer[i]) {
                printf("FAIL (mismatch at index %i - %c vs %c)\n", i, buffer[i], comparisonBuffer[i]);
                checkFail = true;
                break;
            }
        }

        if (!checkFail) printf("PASS\n");

        memset(buffer, 0, test_file->length);
        memset(buffer, 'D', 25);

        printf("\tTesting ext2_write (test.txt, offset 10, size 25)...");
        test_file->write(test_file, 10, 25, buffer);
    
        // Read in the file to check it
        test_file->read(test_file, 10, 25, comparisonBuffer);

        checkFail = false;
        for (int i = 0; i < 25; i++) {
            if (comparisonBuffer[i] != buffer[i]) {
                printf("FAIL (mismatch at index %i - %c vs %c)\n", i, buffer[i], comparisonBuffer[i]);
                checkFail = true;
                break;
            }
        }

        if (!checkFail) printf("PASS\n");

        printf("\tRestoring file contents...");

        test_file->write(test_file, 0, test_file->length, restoreBuffer);

        printf("DONE\n");

        kfree(buffer);

    } else {
        printf("ERROR (ret = %i)\n", ret);
    }

    kfree(restoreBuffer);

    printf("\tTesting ext2_mkdir...");
    ext2_root->mkdir(ext2_root, "test_dir", EXT2_PERM_OR);

    fsNode_t *dir = ext2_root->finddir(ext2_root, "test_dir");
    if (dir != -1) {
        printf("PASS\n");
        kfree(dir);
    } else {
        printf("FAIL\n");
    }

    printf("\tTesting ext2_readdir...");
    struct dirent *direntry = ext2_root->readdir(ext2_root, 0); // I don't honestly know what readdir does
    printf("PASS (ino: %i)\n", direntry->ino);
    
    printf("\tTesting ext2_create...");
    ext2_root->create(ext2_root, "reduceOS.txt", EXT2_PERM_OR);

    fsNode_t *f = ext2_root->finddir(ext2_root, "reduceOS.txt");
    if (f != -1) {
        printf("PASS\n");
        kfree(f);
    } else {
        printf("FAIL (already exists?)\n");
    }

    printf("\tTesting ext2_unlink...");
    ext2_unlink(ext2_root, "reduceOS.txt");
    f = ext2_root->finddir(ext2_root, "reduceOS.txt");

    if (f == -1) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
        kfree(f);
    }

    kfree(test_file);
    kfree(direntry);
    kfree(comparisonBuffer);
    return 0;
}

int bitmap_test() {
    if (!ext2_root) {
        printf("\tEXT2 driver is not running\n");
        return -1;
    }

    printf("\tLocating \"test.bmp\"...");
    fsNode_t *d = ext2_root->finddir(ext2_root, "badapple");
    if (!d) { printf("FAIL\n"); return -1; }
    fsNode_t *image = d->finddir(d, "74.bmp");
    if (!image) { printf("FAIL\n"); return -1; }
    printf("OK\n");

    printf("\tLoading bitmap...");
    
    bitmap_t *bmp = bitmap_loadBitmap(image);
    if (bmp == NULL) {
        printf("FAIL (bmp is NULL)\n");
        return -1;
    } 

    printf("OK\n");


    printf("\tDisplaying bitmap...");
    displayBitmap(bmp, 0, 0);
    sleep(2000);

    kfree(bmp);

    return 0;
}

int badapple_test() {
    // broken :(
    printf("Broken, sorry.\n");
    return -1;

    printf("\tTo run this test, please attach an EXT2 disk to the system.\n");
    printf("\tThe frames should be in a folder called badapple, and be numbered 0-whatever.\n");
    printf("\tNOTE: ALL FRAMES MUST BE BITMAPS, AND START FROM 1. NO ZERO INDEX.\n");
    printf("\tThe test will start in 2 seconds...\n");
    sleep(2000);

    if (!ext2_root) {
        printf("\tEXT2 driver is not running.\n");
        return -1;
    }

    // Find the bad apple directory
    printf("\tLoading directory...");
    fsNode_t *dir = ext2_finddir(ext2_root, "badapple");
    if (!dir) {
        printf("\tFAILED\n");
        return -1;
    } else {
        printf("OK\n");
    }

    serialPrintf("bad_apple: Loaded directory\n");
    

    // Let's do this. First, we need to allocate 2 bitmaps for our frame data to be stored in.
    bitmap_t *fb1;
    bitmap_t *fb2;
    int currentFrame = 1;

    // Now, let's get into a while loop and start displaying frames
    while (true) {
        int tickCount = pitGetTickCount(); // Used for timing our frames

        

        // The way this will work is we'll start the loop by displaying whatever's in fb2
        if (currentFrame > 1) {
            serialPrintf("Displaying next frame...\n");
            displayBitmap(fb2, 0, 0);
            kfree(fb1->buffer);
            kfree(fb2->buffer);
            currentFrame++;
        }        

        // First, we need to get the names of the frames.
        char *fn1 = kmalloc(8); // 4 bytes for '.bmp', 4 bytes for the numbers
        memset(fn1, 0, 8);
        itoa(currentFrame, fn1, 10);
        strcpy(fn1 + strlen(fn1), ".bmp");
        
        currentFrame++;

        char *fn2 = kmalloc(8); // 4 bytes for '.bmp', 4 bytes for the numbers
        memset(fn2, 0, 8);
        itoa(currentFrame, fn2, 10);
        strcpy(fn2 + strlen(fn2), ".bmp");


        serialPrintf("Reading frame '%s'...\n", fn1);

        // Now, we can read in the frames
        fsNode_t *f1 = dir->finddir(dir, fn1);
        if (!f1) {
            kfree(fb1);
            kfree(fb2);
            kfree(fn1);
            kfree(fn2);
            return 0;
        }

        bool breakAfterDisplayOne = false;
        fsNode_t *f2 = dir->finddir(dir, fn2);
        if (!f2) {
            breakAfterDisplayOne = true;
        }

        fb1 = bitmap_loadBitmap(f1);
        if (!breakAfterDisplayOne) fb2 = bitmap_loadBitmap(f2);

        serialPrintf("fb1 loaded to 0x%x buf 0x%x fb2 loaded to 0x%x buf 0x%x\n", fb1,fb1->buffer, fb2, fb2->buffer);
        // NOTE: The system could bug out if an invalid bitmap is passed
        displayBitmap(fb1, 0, 0);
        serialPrintf("Displayed\n");

        if (breakAfterDisplayOne) break; // We have no more bitmaps left to display


        kfree(fn1);
        kfree(fn2);

        // Time ourselves
        // while (pitGetTickCount() - tickCount < 1000);
    }
    return 0;
}

int cpu_tests() {
    cpuInfo_t cpu = getCPUProcessorData();
    printf("\tCPU vendor data: %s\n", getCPUVendorData());
    printf("\tCPU frequency: %i Hz\n", getCPUFrequency());
    printf("\tFPU support (CPUID): %i\n", cpu.fpuEnabled);

    if (cpu.fpuEnabled == 1) {
        float f = 2.430;
        printf("\tFPU test (should be 2.43): %f\n", f);
    }

    printf("\tPIT tick count: %016llX ticks\n", pitGetTickCount());

    return 0;
}




// This function serves as a function to test multiple modules of the OS.
int test(int argc, char *args[]) {
    if (argc != 2) {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32, floppy, ide, fat, ext2, bitmap\n");
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

    } else if (!strcmp(args[1], "ext2")) {
        printf("=== TESTING EXT2 ===\n");

        if (ext2_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "bitmap")) {
        printf("=== TESTING BITMAPS ===\n");

        if (bitmap_test() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n"); 
    } else if (!strcmp(args[1], "bad_apple")) {
        printf("=== bad apple!!! ===\n");

        if (badapple_test() == 0) printf("finished\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "cpu")) {
        printf("=== TESTING PROCESSOR ===\n");

        if (cpu_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else if (!strcmp(args[1], "vfs")) {
        printf("=== TESTING VFS ===\n");

        if (vfs_tests() == 0) printf("=== TESTS COMPLETED ===\n");
        else printf("=== TESTS FAILED ===\n");
    } else {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32, floppy, ide, fat, tree, vfs, ext2, bitmap, badapple, cpu\n");
    }

    return 0;
}

