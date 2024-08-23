// =====================================================
// main.c - Where the magic happens
// =====================================================
// This file is part of the Polyaniline bootloader. Please credit me if you use this code.

#include <boot_terminal.h>
#include <platform.h>
#include <config.h>
#include <multiboot.h>
#include <bootelf.h>

#ifdef EFI_PLATFORM
#include <efi.h>
#endif


// KERNEL VARIABLES, MODIFYME
char *kernel_filename = "KERNEL.ELF";
char *initrd_filename = "RAMDISK.IMG";
// NO LONGER MODIFYME


gdtPtr_t gdtPtr;


// Function prototypes
static int boot();

void bootloader_main() {
    clearScreen();
    draw_polyaniline_testTube();
    boot_printf("polyaniline v%s - codename %s\n", VERSION, CODENAME);
    
    int ret = boot();
    
    setColor(0x04);
    boot_printf("boot() did not succeed - return code %i\n", ret);
    boot_printf("Failed to load reduceOS. Halting system.\n");
    for (;;);
}

// checkKernelMultiboot() - Scans the kernel's load address for a multiboot signature
int checkKernelMultiboot() {
    for (int x = 0; x < KERNEL_PAGES; x++) {
        // It'll be at the beginning of a page
        uint32_t *value = (uint32_t*)(KERNEL_LOAD_ADDR + x);
        if (*value == 0x1BADB002) {
            // Found the multiboot header, but we need to check if it is an a.out kludge style program, or an ELF32
            // We don't support a.out yet

            // Check if the bit 16 is set in MULTIBOOT_FLAGS.
            if (value[1] & (1 << 16)) {
                setColor(0x04);
                boot_printf("a.out formatted kernels are not currently supported.\n");
                return -1;
            } else {
                boot_printf("Verified kernel successfully.\n");
                return 1;
            }
        }
    }

    setColor(0x04);
    boot_printf("No multiboot structure was found - kernel invalid.\n");
    return 0;
}


#ifdef EFI_PLATFORM
/* EFI CODE */

// Function prototypes
UINTN loadKernel(EFI_FILE *kernel);
UINTN loadRamdisk(EFI_FILE *ramdisk, UINTN bytes);
int efi_finish();

// UEFI uses a simple filesystem driver - you can read up on it at uefi.org.
// We need two GUIDs - one for EFI_LOADED_IMAGE_PROTOCOL and one for EFI_LOAD_FILE_PROTOCOL
// Normally, you could just use EFI_LOADED_IMAGE_PROTOCOL_GUID and others but it's broken for some reason, so we can't do that.
// No problem - we'll just define our own
static EFI_GUID efi_loaded_image_protocol_guid = {0x5B1B31A1,0x9562,0x11d2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}};
static EFI_GUID efi_simple_filesystem_guid = {0x0964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static EFI_GUID efi_serial_io_protocol_guid = {0xBB25CF6F,0xF1D4,0x11D2, {0x9a,0x0c,0x00,0x90,0x27,0x3f,0xc1,0xfd}};
 

// Variables
EFI_LOADED_IMAGE *loaded_image;
EFI_FILE_IO_INTERFACE *fileIO;
EFI_SERIAL_IO_PROTOCOL *serial;
EFI_FILE_PROTOCOL *root;
uintptr_t initrd_offset;
uintptr_t initrd_length;
uintptr_t elf_entrypoint;
uintptr_t elf_end;


int serialEnabled = 0;

// Externals
extern EFI_SYSTEM_TABLE *ST;
extern EFI_HANDLE Image_Handle;


// efi_init_serial() - Startup the serial wrapper
EFI_STATUS efi_init_serial() {
    // Grab a handle to the serial I/O protocol
    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 2, &efi_serial_io_protocol_guid, (void**)&serial);
    if (EFI_ERROR(status)) {
        return status;
    }

    serial->Write(serial, 6, "HELLO");
}

// getFilename() - Helper function to convert char* to CHAR16 (todo: move me to utility.c)
void getFilename(CHAR16 *output, char *input) {
    char *c = kernel_filename;
    char *ascii = c;
    int i = 0;
    while (*ascii) {
        output[i] = *ascii;
        i++;
        ascii++;
    }
        
    if (output[i-1] == L'.') {
        output[i-1] = 0;
    }
}


int boot() {
    // First, setup a watchdog timer
    uefi_call_wrapper(ST->BootServices->SetWatchdogTimer, 4, 0, 0, 0, NULL);

    // Now, let's initialize serial.
    if (EFI_ERROR(efi_init_serial())) {
        boot_printf("Could not start serial I/O device - continuing anyways.\n");
        serialEnabled = 0;
    } else {
        boot_printf("Initialized serial I/O logging\n");
    }

    // We need to load in the image.
    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, Image_Handle, &efi_loaded_image_protocol_guid, (void**)&loaded_image);
    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to load image\n");
        return -1;
    }
    
    boot_printf("Image loaded successfully.\n");
    
    // Now, we need a filesystem protocol.
    status = uefi_call_wrapper(ST->BootServices->HandleProtocol, 3, loaded_image->DeviceHandle, &efi_simple_filesystem_guid, (void**)&fileIO);

    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to load filesystem - status %i\n", status);
        return -1;
    }

    boot_printf("Filesystem loaded successfully.\n");


    // Open up the volume
    status = uefi_call_wrapper(fileIO->OpenVolume, 2, fileIO, (void**)&root);
    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to open root volume in EFI system partition\n");
        return -1;
    }

    // Spent quite a while trying to figure this out, but uefi_call_wrapper will ONLY accept a CHAR16.
    // strcpy() does not support CHAR16, so we have to do it this way. We use the period as a quick hack.
    CHAR16 kfile[16] = {0};
    getFilename(&kfile, kernel_filename);
    
    EFI_FILE *kernel;
    status = uefi_call_wrapper(root->Open, 5, root, &kernel, kfile, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Could not load the kernel.\n");
        return -1;
    }

    // Load the kernel
    UINTN loadAddr = loadKernel(kernel);
    if (loadAddr == 0) return -1;
    boot_printf("Kernel loaded successfully.\n");

    // Load the ramdisk
    CHAR16 rfile[16] = {0};
    getFilename(&rfile, initrd_filename);
    
    EFI_FILE *initrd;
    status = uefi_call_wrapper(root->Open, 5, root, &initrd, rfile, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Could not load the initial ramdisk.\n");
        return -1;
    }

    // Load the ramdisk
    UINTN ramdisk_loadAddr = loadRamdisk(initrd, loadAddr);
    if (ramdisk_loadAddr == 0) return -1;
    boot_printf("Ramdisk loaded successfully.\n");


    // Now that we've done that, we need to check the kernel. We'll call into a global function called checkKernelMultiboot.
    if (checkKernelMultiboot() != 1) {
        return -1;
    }

    // Let's load the ELF file
    Elf32_Ehdr *ehdr = (Elf32_Ehdr*)KERNEL_LOAD_ADDR;
    uintptr_t end = loadELF(ehdr);
    if (end == 0x0) return -1;

    uintptr_t entrypoint = ehdr->e_entry;
    boot_printf("Kernel validated successfully. Entrypoint: 0x%x. End: 0x%x\n", entrypoint, end);

    elf_entrypoint = entrypoint;
    elf_end = end;

    // Alright, we're about ready to load the kernel. We just need to finish up by calling efi_finish() to get multiboot information.
    efi_finish();

    return 0;
}

UINTN loadKernel(EFI_FILE *kernel) {
    // We need to load the kernel into memory.

    // Let's alloacte some pages (quite a lot, actually!)
    EFI_PHYSICAL_ADDRESS addr = KERNEL_LOAD_ADDR;
    EFI_ALLOCATE_TYPE type = AllocateAddress;
    EFI_MEMORY_TYPE memtype = EfiLoaderData;
    UINTN pages = KERNEL_PAGES;
    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->AllocatePages, 4, type, memtype, pages, &addr);
    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to allocate memory to load kernel. Error code: %i\n", status);
        boot_printf("Attempted load address: 0x%x\n", addr);
        return 0;
    }

    // Now, we'll read in kernel.elf. We'll use 128MB as the buffer size because it'll automatically be limited anyways.
    UINTN bufferSize = LOAD_SIZE; 
    status = uefi_call_wrapper(kernel->Read, 3, kernel, &bufferSize, (void*)KERNEL_LOAD_ADDR);
    
    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to read in kernel file.\n");
        return 0;
    }

    // All done
    return bufferSize; // bufferSize will be updated by the uefi_call_wrapper()
}

UINTN loadRamdisk(EFI_FILE *ramdisk, UINTN bytes) {
    // We need to load the ramdisk into memory, but we have to calculate it based off bytes, unfortunately.
    UINTN ramdisk_bytes = LOAD_SIZE;

    // Convert bytes to the pages
    UINTN offset = bytes;
    while (offset % 4096) offset++; // 4096 is page size

    // Let's load in the file
    EFI_STATUS status = uefi_call_wrapper(ramdisk->Read, 3, ramdisk, &ramdisk_bytes, (void*)(uintptr_t)(KERNEL_LOAD_ADDR + offset));

    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to read in initial ramdisk.\n");
        return 0;
    } else {
        initrd_length = ramdisk_bytes;
        initrd_offset = KERNEL_LOAD_ADDR + offset;
    }

    return initrd_length;
}

int elf_readMemoryMap(multiboot_info *info, memoryRegion_t *map) {
    UINTN size = 0;
    UINTN mm_key = 0;
    UINTN descriptor_size; // Might be differing versions

    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5, &size, NULL, &mm_key, &descriptor_size, NULL);

    EFI_MEMORY_DESCRIPTOR *efi_memoryMap = (void*)elf_end;
    elf_end += size;
    while ((uintptr_t)elf_end & 0x3FF) elf_end++; // Aligning again

    // Now let's actually call GetMemoryMap an dparse he descriptors
    status = uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5, &size, efi_memoryMap, &mm_key, &descriptor_size);
    if (EFI_ERROR(status)) {
        setColor(0x04);
        boot_printf("Failed to get memory map.\n");
        return -1;
    }

    // We bascially have to translate all EFI descriptors to a multiboot-type memory map, with types reserved and setup
    // There's no good way to do this, but I'll try to keep it clean
    uint64_t upperMemory = 0; // Calculated based off if the memory is available
    int descriptorCount = size / descriptor_size;

    // Iterate through descriptors
    for (int descriptor_index = 0; descriptor_index < descriptorCount; descriptor_index++) {
        EFI_MEMORY_DESCRIPTOR *desc = efi_memoryMap;
        map->address = desc->PhysicalStart;
        map->size = sizeof(memoryRegion_t) - sizeof(uint32_t);
        map->length = desc->NumberOfPages * 0x1000;

        // Check the type, we'll mark it as the proper type.
        /*
            Quick refresher on memory types:
            - Available = 1
            - Reserved = 2
            - ACPI Reclaimable = 3
            - ACPI NVS Memory = 4
        */

       switch (desc->Type) {
        // Available memory
        case EfiConventionalMemory:
        case EfiLoaderCode:             // Allow reduceOS to automatically reclaim the EFI loader's memory
        case EfiLoaderData:
        case EfiBootServicesCode:       // Because reduceOS is an OS that boots based off of multiboot and not EFI boot services, we can ignore this
        case EfiBootServicesData:
            map->type = 1;
            break;
        
        // We'll handle the ACPI types
        case EfiACPIReclaimMemory:
            map->type = 3;
            break;
        case EfiACPIMemoryNVS:
            map->type = 4;
            break;
        
        // Now for the reserved stuff
        case EfiReservedMemoryType:
        case EfiUnusableMemory:
        case EfiMemoryMappedIO:
        case EfiMemoryMappedIOPortSpace:
        case EfiPalCode:
        case EfiRuntimeServicesCode:    // TODO: EFI documentation lists these as "reserved" but are they actually
        case EfiRuntimeServicesData:
        default:
            map->type = 2;
            break;
        }
    
        // Check if the type was marked as available and if its greater than our kernel's actual load address
        if (map->type == 1 && map->address >= 0x100000) {
            upperMemory += map->length;
        }
        
        // Now we can increment the memory map
        map = (memoryRegion_t*)((uintptr_t)map + map->size + sizeof(uint32_t));
        efi_memoryMap = (EFI_MEMORY_DESCRIPTOR*)((char*)efi_memoryMap + descriptor_size);
    }
    // Now, finish it off by using our values to fill in mem_lower and mem_upper

    info->m_mmap_length = (uintptr_t)map - info->m_mmap_addr;
    info->m_memoryLo = 1024;                // 1 MB in KB, load address of kernel
    info->m_memoryHi = upperMemory / 1024;  // upperMemory is in bytes, convert it to KB

    return 0;
}


int efi_finish() {

    // We'll have to create the multiboot header and append it to elf_end
    multiboot_info *info = (void*)(uintptr_t)elf_end;
    elf_end += sizeof(multiboot_info);

    // Setup flags - CMDLINE | MODS | MEM | MMAP | LOADER
    info->m_flags = 0x2004D;

    // Framebuffer won't be setup for now
    char *cmdline = "hello_kernel";
    memcpy((void*)elf_end, cmdline, strlen(cmdline) + 1);
    info->m_cmdLine = cmdline;
    elf_end += strlen(cmdline) + 1;


    // Copy in the bootloader name
    char *bname = "hello_kernel";
    memcpy((void*)elf_end, bname, strlen(bname) + 1);
    info->m_bootloader_name = bname;
    elf_end += strlen(bname) + 1;

    // Copy over modules
    multiboot_mod_t mod_initial = {
        .mod_start = 0,
        .mod_end = 0,
        .cmdline = "modfs=1 type=initrd",
        .padding = 1
    };
    memcpy((void*)elf_end, &mod_initial, sizeof(multiboot_mod_t));
    info->m_modsAddr = (uintptr_t)elf_end;
    // info->m_modsCount++;
    elf_end += sizeof(multiboot_mod_t);

    // Realign the offset to nearest page for the memory map
    elf_end = (elf_end & ~(0xFFF)) + ((elf_end & 0xFFF) ? 0x1000 : 0);

    // Now, we'll create our dummy memory map
    memoryRegion_t *region = (void*)elf_end;
    memset((void*)elf_end, 0x00, 1024);
    info->m_mmap_addr = (uint32_t)(uintptr_t)region;

    // Now we'll read in the memory map by calling elf_readMemoryMap
    int ret = elf_readMemoryMap(info, region);
    if (ret != 0) {
        return -1;
    }

    // Let's move the initial ramdisk to a different location
    char *initrd_dest = (char*)elf_end;
    char *initrd_src = (char*)initrd_offset;
    for (int s = 0; s < initrd_length; s++) initrd_dest[s] = initrd_src[s];
    multiboot_mod_t *mod = (multiboot_mod_t*)info->m_modsAddr;

    mod->mod_start = elf_end;
    mod->mod_end = elf_end + initrd_length;
    
    // Add to elf_end & align it again
    elf_end += initrd_length;
    elf_end = (elf_end & ~(0xFFF)) + ((elf_end & 0xFFF) ? 0x1000 : 0);

    // We're in the home stretch now.
    // Exit boot services and GO!

    // We have to get map key from memory map first.
    UINTN mapKey, mapSize, descriptor_size;
    uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5, &mapSize, NULL, &mapKey, &descriptor_size, NULL);
    
    // Now, let's exit boot services and jump.
    EFI_STATUS status = uefi_call_wrapper(ST->BootServices->ExitBootServices, 2, Image_Handle, mapKey);
    if (status != EFI_SUCCESS) {
        boot_printf("Failed to exit boot services.\n");
        return -1;
    }
    

    
}


#else
/* BIOS CODE*/
#endif
