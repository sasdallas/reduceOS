// test.c - Test command for reduceOS

#include "include/test.h"

#include "include/libc/stdint.h" 
#include "include/libc/string.h"

#include "include/fat.h"
#include "include/serial.h"
#include "include/terminal.h"
#include "include/floppy.h"


extern ideDevice_t ideDevices[4];
extern fsNode_t *fatDriver;

// This function is hopefully going to serve as a function to test multiple modules of the OS.
int test(int argc, char *args[]) {
    if (argc != 2) { printf("Usage: test <module>\n"); return -1; }

    if (!strcmp(args[1], "pmm")) {
        printf("=== TESTING PHYSICAL MEMORY MANAGEMENT ===\n");
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

        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "liballoc")) {
        printf("=== TESTING LIBALLOC ===\n");
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
        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "bios32")) {
        printf("=== TESTING BIOS32 ===\n");

        printf("\tServing INT 0x15...\n");
        REGISTERS_16 in, out = {0};

        in.ax = 0xE820;
        bios32_call(0x15, &in, &out);

        printf("\tInterrupt serviced. Results:\n");
        printf("\tAX = 0x%x BX = 0x%x CX = 0x%x DX = 0x%x\n", out.ax, out.bx, out.cx, out.dx);

        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "floppy")) {
        printf("=== TESTING FLOPPY ===\n");

        
        uint32_t sector = 0;
        uint8_t *buffer = 0;

        printf("\tReading sector %i...\n", sector);

        int ret = floppy_readSector(sector, &buffer);
    
        if (ret != FLOPPY_OK) {
            printf("Could not read sector. Error code %i\n", ret);
            printf("=== TESTS FAILED ===\n");
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

        printf("=== TESTS COMPLETED ===\n");
    } else if (!strcmp(args[1], "ide")) {
        printf("=== TESTING IDE ===\n");
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
            printf("=== TESTS FAILED ===\n");
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
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i]); } 
            }

            if (match) printf("PASS\n");
        }

        memset(sector, 0, 1120);
        printf("\tTesting VFS node read (offset 512, size 1120)...");
        error = ret->read(ret, 512, 1120, sector);

        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i + 512]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i + 512]); break; } 
            }

            if (match) printf("PASS\n");
        }


        memset(sector, 0, 1120);
        printf("\tTesting VFS node read (offset 723, size 1120)...");
        
        error = ret->read(ret, 723, 1120, sector);

        if (error != IDE_OK) {
            printf("FAIL (function returned %i)\n", error);
        } else {
            // Check if the sector matches the comparison buffer
            bool match = true;
            for (int i = 0; i < 1120; i++) {
                if (sector[i] != comparison_buffer[i + 723]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], comparison_buffer[i + 723]); break;} 
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
        } else {
            // Read in and check the sector
            error = ret->read(ret, 0, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); break;}
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
        } else {
            // Read in and check the sector
            error = ret->read(ret, 80, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); break;}
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
        } else {
            // Read in and check the sector
            error = ret->read(ret, 763, 600, sector_comparison);
            if (error != IDE_OK) {
                printf("FAIL (could not read for checking, returned %i)\n", error);
            } else {
                // Check if the sector matches the comparison buffer
                bool match = true;
                for (int i = 0; i < 600; i++) {
                    if (sector[i] != sector_comparison[i]) { match = false; printf("FAIL (mismatch at index %i - 0x%x vs 0x%x)\n", i, sector[i], sector_comparison[i]); break;}
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



        printf("=== TESTS COMPLETED ===\n");

    } else if (!strcmp(args[1], "fat")) {
        printf("=== TESTING FAT DRIVER ===\n"); 

        

        if (!isFatRunning()) {
            printf("\tFAT driver is not running\n");    
            printf("=== TESTS FAILED ===\n");
            return -1;
        }

        
        printf("\tTesting fatOpenInternal (test.txt)...");

        fsNode_t ret = fatOpenInternal(fatDriver, "/test.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
        } else {
            printf("PASS\n");
        }


        printf("\tTesting fatOpenInternal (dir/test.txt)...");

        ret = fatOpenInternal(fatDriver, "/dir/test.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
        } else {
            printf("PASS\n");
        }

        printf("\tTesting fatOpenInternal (nonexistent.txt)...");

        ret = fatOpenInternal(fatDriver, "/nonexistent.txt");
        if (ret.flags != -1) {
            printf("FAIL (flags = 0x%x)\n", ret.flags);
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
            }
            kfree(buffer);
        }

        printf("\tTesting fatReadInternal (cluster.txt, 4 clusters)...");

        ret = fatOpenInternal(fatDriver, "/cluster.txt");
        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
        } else {
            buffer = kmalloc(4 * 4 * 512);
            
            bool fail = false;
            int returnValue;
            for (int i = 0; i < 4; i++) {
                if (returnValue == EOF) { 
                    printf("FAIL (file spans <4 clusters)\n" );
                    fail = true;
                    break;
                }
                returnValue = fatReadInternal(&ret, buffer + (i*4*512), ret.length); // Bugged bc we dont know sectors per cluster so we assume 4
            }

            if (returnValue != EOF) {
                printf("FAIL (file spans >4 clusters)\n");
            } else if (!fail) {
                printf("PASS\n");
            }

            kfree(buffer);
        }
    
        printf("\tReading in test.txt for next set of tests...");
        ret = fatOpenInternal(fatDriver, "/test.txt");
        uint8_t *comparison_buffer = kmalloc(ret.length);

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
        } else {
            fatReadInternal(&ret, comparison_buffer, ret.length);
            printf("DONE\n");
        }

        printf("\tTesting fatRead (test.txt, offset 0, size 100)...");
        

        if (ret.flags != VFS_FILE) {
            printf("FAIL (fatOpenInternal failed)\n");
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
                        break;
                    }
                }

                if (success) printf("PASS\n");
            }
            kfree(buffer);
        }

        kfree(comparison_buffer);
        // kfree(ret); - Glitches out??

        printf("=== TESTS COMPLETED ===\n");
        
    } else {
        printf("Usage: test <module>\n");
        printf("Available modules: pmm, liballoc, bios32, floppy, ide, fat\n");
    }

    return 0;
}