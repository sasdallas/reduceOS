#include "kernel.h" // Kernel headers
#include "console.h" // Console handling
#include "string.h" // Memory and String handling
#include "gdt.h" // Global Descriptor Table
#include "idt.h" // Interrupt Descriptor Table
#include "keyboard.h" // Keyboard driver
#include "vga.h" // VGA driver
#include "timer.h" // IRQ Timer
#include "ide.h" // IDE driver
#include "multiboot.h" // Multiboot parameters and stuff


void __cpuid(uint32_t type, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
	asm volatile("cpuid"
				: "=a"(*eax), "=b"(*ebx), "=c"(ecx), "=d"(*edx)
				: "0"(type)); // Add type to ea
}














void getMemInfo(unsigned long magic, unsigned long addr) {
	MULTIBOOT_INFO *mboot_info;
	uint32_t i;

	printf("Magic: 0x%x\n", magic);
	if (magic == MULTIBOOT_BOOTLOADER_MAGIC) { // Magic matches the bootloader magic
		mboot_info = (MULTIBOOT_INFO *)addr;
		printf("	flags: 0x%x\n", mboot_info->flags);
		printf("	low mem: 0x%x KB\n", mboot_info->mem_low);
		printf("	high mem: 0x%x KB\n", mboot_info->mem_high);
		printf("	boot device: 0x%x\n", mboot_info->boot_device);
		printf("	cmdline: %s\n",(char *)mboot_info->cmdline);
		printf("	modules amnt: %d\n", mboot_info->modules_count);
		printf("	modules addr: 0x%x\n", mboot_info->modules_addr);
		printf("	mmap length: %d\n", mboot_info->mmap_length);
		printf("	mmap addr: 0x%x\n", mboot_info->mmap_addr);
		printf("	memory map:-\n");
		for (i = 0; i < mboot_info->mmap_length; i += sizeof(MULTIBOOT_MEMORY_MAP)) {
            MULTIBOOT_MEMORY_MAP *mmap = (MULTIBOOT_MEMORY_MAP *)(mboot_info->mmap_addr + i);
            printf("    size: %d, addr: 0x%x%x, len: %d%d, type: %d\n", 
                    mmap->size, mmap->addr_low, mmap->addr_high, mmap->len_low, mmap->len_high, mmap->type);

            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                /**** Available memory  ****/
            }
        }
		printf("  boot_loader_name: %s\n", (char *)mboot_info->boot_loader_name);

	} else {
		printf("ERROR: invalid multiboot magic number\n");
	}
}





void getCPUIDInfo() {
	uint32_t brand[12];
	uint32_t eax, ebx, ecx, edx;
	uint32_t type;

	memset(brand, 0, sizeof(brand));
	__cpuid(0x80000002, (uint32_t *)brand+0x0, (uint32_t *)brand+0x1, (uint32_t *)brand+0x2, (uint32_t *)brand+0x3);
    __cpuid(0x80000003, (uint32_t *)brand+0x4, (uint32_t *)brand+0x5, (uint32_t *)brand+0x6, (uint32_t *)brand+0x7);
    __cpuid(0x80000004, (uint32_t *)brand+0x8, (uint32_t *)brand+0x9, (uint32_t *)brand+0xa, (uint32_t *)brand+0xb);
	printf("System brand: %s\n", brand);
	for (type = 0; type < 4; type++) {
		__cpuid(type, &eax, &ebx, &ecx, &edx);
		printf("Type:0x%x, eax: 0x%x, ebx: 0x%x, ecx: 0x%x, edx:0x%x\n", type, eax, ebx, ecx, edx);
	}
}
BOOL is_echo(char *b) {
    if((b[0]=='e')&&(b[1]=='c')&&(b[2]=='h')&&(b[3]=='o'))
        if(b[4]==' '||b[4]=='\0')
            return TRUE;
    return FALSE;
}



BOOL is_color(char *b) {
	if ((b[0]=='s')&&(b[1]=='e')&&(b[2]=='t')&&(b[3]=='c')&&(b[4]=='o')&&(b[5]=='l')&&(b[6]=='o')&&(b[7]=='r'))
		if (b[8]==' '||b[8]=='\0')
			return TRUE;
	return FALSE;
}
BOOL is_drive(char *b) {
	if ((b[0]=='s')&&(b[1]=='e')&&(b[2]=='l')&&(b[3]=='d')&&(b[4]=='r')&&(b[5]=='i')&&(b[6]=='v')&&(b[7]=='e'))
		if (b[8] == ' '||b[8]=='\0')
			return TRUE;
	return FALSE;
}
BOOL is_writedrive(char *b) {
	if ((b[0]=='w')&&(b[1]=='r')&&(b[2]=='i')&&(b[3]=='t')&&(b[4]=='e')&&(b[5]=='d')&&(b[6]=='r')&&(b[7]=='i')&&(b[8]=='v')&&(b[9]=='e')) 
		if (b[10]==' '||b[10]=='\0')
			return TRUE;
	return FALSE;
}



// This uses an MBR written by egormkn, thanks to him.
// Function purpose: to test installing an MBR to a sector of a disk

// This test bootloader was written by egormkn, check the code at https://github.com/egormkn/mbr-boot-manager/blob/master/mbr.asm

void installMBR(int DRIVE) {
	printf("Installing MBR to sector 0...");
	const uint32_t LBA = 0;
    const uint8_t NO_OF_SECTORS = 1;
    
	char buf[] = {
  0xfa, 0xbc, 0x00, 0x7c, 0x31, 0xc0, 0x8e, 0xd0, 0x8e, 0xc0, 0x8e, 0xd8,
  0x52, 0xbe, 0x00, 0x7c, 0xbf, 0x00, 0x06, 0xb9, 0x00, 0x02, 0xfc, 0xf3,
  0xa4, 0xe9, 0x00, 0x8a, 0xfb, 0xb8, 0x03, 0x00, 0xcd, 0x10, 0xb8, 0x00,
  0x06, 0xb7, 0x02, 0x31, 0xc9, 0xba, 0x4f, 0x18, 0xcd, 0x10, 0xb8, 0x03,
  0x01, 0xb9, 0x05, 0x01, 0xcd, 0x10, 0xb9, 0x04, 0x00, 0xbd, 0xee, 0x07,
  0x31, 0xdb, 0x55, 0x80, 0x7e, 0x00, 0x80, 0x75, 0x02, 0x88, 0xcb, 0xb4,
  0x02, 0xba, 0x22, 0x08, 0x00, 0xce, 0xcd, 0x10, 0xbe, 0x8d, 0x07, 0xe8,
  0x2b, 0x01, 0xb0, 0x30, 0x00, 0xc8, 0xb4, 0x0e, 0xcd, 0x10, 0x38, 0xcb,
  0x75, 0x06, 0xbe, 0x98, 0x07, 0xe8, 0x19, 0x01, 0x5d, 0x83, 0xed, 0x10,
  0xe2, 0xd0, 0x38, 0xfb, 0x75, 0x03, 0x43, 0xeb, 0x0a, 0xb4, 0x02, 0xcd,
  0x16, 0x24, 0x03, 0x38, 0xf8, 0x74, 0x51, 0xb4, 0x02, 0xba, 0x20, 0x08,
  0x00, 0xde, 0xcd, 0x10, 0x88, 0xfc, 0xcd, 0x16, 0x3d, 0x00, 0x48, 0x74,
  0x11, 0x3d, 0x00, 0x50, 0x74, 0x14, 0x3d, 0x1b, 0x01, 0x74, 0x2d, 0x3d,
  0x0d, 0x1c, 0x74, 0x30, 0xeb, 0xdd, 0x80, 0xfb, 0x01, 0x7e, 0x01, 0x4b,
  0xeb, 0xd5, 0x80, 0xfb, 0x04, 0x73, 0x01, 0x43, 0xeb, 0xcd, 0xbe, 0x9d,
  0x07, 0xe8, 0xc9, 0x00, 0xb8, 0x00, 0x86, 0xb9, 0x2d, 0x00, 0x31, 0xd2,
  0xcd, 0x15, 0xeb, 0x04, 0xcd, 0x18, 0xcd, 0x19, 0xea, 0x00, 0x00, 0xff,
  0xff, 0xf4, 0xeb, 0xfd, 0x53, 0xb4, 0x02, 0xba, 0x01, 0x01, 0xcd, 0x10,
  0xb8, 0x00, 0x06, 0xb7, 0x02, 0x31, 0xc9, 0xba, 0x4f, 0x18, 0xcd, 0x10,
  0xbd, 0xae, 0x07, 0x5b, 0xc1, 0xe3, 0x04, 0x01, 0xdd, 0x5a, 0x88, 0x56,
  0x00, 0x55, 0xc6, 0x46, 0x11, 0x05, 0x88, 0x7e, 0x10, 0xb4, 0x41, 0xbb,
  0xaa, 0x55, 0xcd, 0x13, 0x5d, 0x72, 0x0f, 0x81, 0xfb, 0x55, 0xaa, 0x75,
  0x09, 0xf7, 0xc1, 0x01, 0x00, 0x74, 0x03, 0xfe, 0x46, 0x10, 0x66, 0x60,
  0x80, 0x7e, 0x0a, 0x00, 0x74, 0x20, 0x66, 0x6a, 0x00, 0x66, 0xff, 0x76,
  0x08, 0x6a, 0x00, 0x68, 0x00, 0x7c, 0x6a, 0x01, 0x6a, 0x10, 0xb4, 0x42,
  0x8a, 0x56, 0x00, 0x89, 0xe6, 0xcd, 0x13, 0x9f, 0x83, 0xc4, 0x10, 0x9e,
  0xeb, 0x14, 0xb8, 0x01, 0x02, 0xbb, 0x00, 0x7c, 0x8a, 0x56, 0x00, 0x8a,
  0x76, 0x01, 0x8a, 0x4e, 0x02, 0x8a, 0x6e, 0x03, 0xcd, 0x13, 0x66, 0x61,
  0x73, 0x12, 0xfe, 0x4e, 0x11, 0x0f, 0x84, 0x59, 0xff, 0x55, 0x30, 0xe4,
  0x8a, 0x56, 0x00, 0xcd, 0x13, 0x5d, 0xeb, 0xae, 0x81, 0x3e, 0xfe, 0x7d,
  0x55, 0xaa, 0x0f, 0x85, 0x44, 0xff, 0x83, 0x3e, 0x00, 0x7c, 0x00, 0x0f,
  0x84, 0x3b, 0xff, 0x8b, 0x56, 0x00, 0x30, 0xf6, 0xea, 0x00, 0x7c, 0x00,
  0x00, 0xb4, 0x0e, 0xac, 0x3c, 0x00, 0x74, 0x04, 0xcd, 0x10, 0xeb, 0xf7,
  0xc3, 0x50, 0x61, 0x72, 0x74, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x00,
  0x20, 0x28, 0x41, 0x29, 0x00, 0x42, 0x6f, 0x6f, 0x74, 0x20, 0x73, 0x65,
  0x63, 0x74, 0x6f, 0x72, 0x20, 0x65, 0x72, 0x72, 0x6f, 0x72, 0x0d, 0x0a,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa
	};
	unsigned int mbr_bin_len = 512;

	printf("Writing...\n");
	ide_write_sectors(DRIVE, NO_OF_SECTORS, LBA, (uint32_t)buf);
	printf("Written.\n");
	printf("Testing...\n");

	char buf2[] = {0};
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA, (uint32_t)buf2);

	
  return 1;
}

void doWriteDrive(char *b, int drive) {
	if (drive == -2) {
		printf("Please select a drive first.\n");
		return;
	}
	char *confirm = "Are you sure you wish to write to this drive? [Y/N] ";
	printf(confirm);
	char buf[255];
	memset(buf, 0, sizeof(buf));
	int idToWriteTo = -1;
	char *data;

	getStringBound(buf, sizeof(confirm));
	if (strcmp(buf, "y") == 0) {
		confirm = "Write to ID: ";
		printf(confirm);
		memset(buf,0,sizeof(buf));
		getStringBound(buf, sizeof(confirm));
		printf("Writing to ID %s", buf);
		//idToWriteTo = atoi(buf);
/*		if (atoi(buf) == 0) {
			printf("ERROR: Enter a valid ID next time.\n");
			return;

		}

		confirm = "Data to write: ";
		printf(confirm);
		memset(buf,0,sizeof(buf));
		getStringBound(buf, sizeof(confirm));

		printf("Writing data %s to ID %d..\n", buf, idToWriteTo);
		*/
	}
}



void kernel_main(unsigned long magic, unsigned long addr) {
	gdt_init();
	idt_init();
	initConsole(COLOR_WHITE, COLOR_BLUE);
	keyboard_init();
	ata_init();
	char buffer[255];
	const char *shell = ">";
	

	printf("reduceOS v0.3 loaded\n");
	printf("Type help for help...\n");


	int DRIVE = -2; // When a drive is not found, the method get_drive_model_by_model will return -1. So, if the user has not even selected a drive, it will be -2.
	
	while (1) {
		printf(shell); // Print basic shell
		memset(buffer, 0, sizeof(buffer)); // memset buffer to 0
		getStringBound(buffer, strlen(shell));
		if (strlen(buffer) == 0)
			continue;
		if (strcmp(buffer, "about") == 0) {
			printf("reduceOS v0.2\n");
			printf("Build 5\n");
		} else if (strcmp(buffer, "getcpuid") == 0) {
			getCPUIDInfo();
		} else if (strcmp(buffer, "help") == 0) {
			printf("reduceOS shell v0.1\n");
			printf("Commands: help, getcpuid, echo, about, clear, meminfo, listdrives, seldrive, color, setup\n");
		} else if (is_echo(buffer)) {
			printf("%s\n", buffer + 5);
		} else if (strcmp(buffer, "clear") == 0) {
			clearConsole(COLOR_WHITE, COLOR_BLUE);
			printf("Cleared console.\n");
		} else if (strcmp(buffer, "meminfo") == 0) {
			getMemInfo(magic, addr);
		} else if (strcmp(buffer, "setcolor") == 0) {
			setColor(COLOR_RED, COLOR_BLUE);
		} else if (strcmp(buffer, "listdrives") == 0) {
			listDrives();
		} else if (is_drive(buffer)) {
			printf("Checking for drive %s...\n", buffer+9);
			DRIVE = ata_get_drive_by_model(buffer+9);
			if (DRIVE == -1) {
				printf("ERROR: No drive with model %s found.\n");
			} 
			
		} else if (strcmp(buffer, "setup") == 0) {
			printf("Loading reduceOS setup ALPHA...\n");
			if (DRIVE == -2 || DRIVE == -1) {
				printf("Please choose a drive first.\n");
			} else {
				installMBR(DRIVE);
			}
		} else if (is_writedrive(buffer)) {
			doWriteDrive(buffer, DRIVE);
		} else {
			printf("Command not found: %s\n", buffer);
		}
	}

	
}
