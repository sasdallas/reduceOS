// ===========================================================
// module.c - reduceOS module loader
// ===========================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/module.h>
#include <kernel/mod.h>
#include <kernel/vfs.h>
#include <kernel/elf.h>

// Modules are NEVER unloaded, and we always will have them start at MODULE_ADDR_START (default is 0xA0000000)
uint32_t *last_load_address = MODULE_ADDR_START; // Incremented when a module is loaded
hashmap_t *module_hashmap = NULL;
char *moduser_buf = NULL;



// module_load(fsNode_t *modfile, int argc, char **args, struct Metadata *mdataout) - Loads a module for modfile
int module_load(fsNode_t *modfile, int argc, char **args, struct Metadata *mdataout) {
    if (!modfile) {
        serialPrintf("module_load: Incorrect parameters specified.\n");
        return MODULE_PARAM_ERROR;
    }


    // A bit hacky, but it works lol
    // We'll allocate physical memory using our PMM, then map it into the last_load_address
    // Round the length to the nearest 4096
    uint32_t length = modfile->length + (4096 - (modfile->length % 4096));
    void *mem = pmm_allocateBlocks(length);
    for (uint32_t i = 0, paddr=mem, vaddr=last_load_address; i < length / 0x1000; i++, paddr += 0x1000, vaddr += 0x1000) {
        vmm_mapPage(paddr, vaddr);
    }

    // Read in the module
    int retval = modfile->read(modfile, 0, modfile->length, (uint8_t*)last_load_address);
    if (retval != modfile->length) {
        pmm_freeBlocks(mem, length);
        return MODULE_READ_ERROR;
    }


    // We'll have to call the load buffer function for ELF
    void *ret = elf_loadFileFromBuffer(last_load_address);
    if (ret != 0x0) {
        serialPrintf("module_load: Could not load module\n");
        return MODULE_LOAD_ERROR;
    }

    // Find the metadata symbol
    struct Metadata *data = elf_findSymbol((Elf32_Ehdr*)last_load_address, "data");
    if (data == NULL) {
        pmm_freeBlocks(mem, length);
        return MODULE_META_ERROR;
    }

    // Check if data exists
    if (hashmap_has(module_hashmap, data->name)) {
        // Unmap the module
        serialPrintf("module_load: Module already loaded into memory\n");
        pmm_freeBlocks(mem, length);
        return MODULE_EXISTS_ERROR;
    }


    serialPrintf("module_load: Loading module '%s'...\n", data->name);
    int status = data->init(argc, args);

    if (status != 0) {
        // Module failed to initialize, log the error and free it
        serialPrintf("module_load: Module '%s' failed to load correctly.\n", data->name);
        pmm_freeBlocks(mem, length);
        return MODULE_INIT_ERROR;
    }

    // We did it! Construct a loaded module first
    loaded_module_t *loadedModule = kmalloc(sizeof(loaded_module_t));
    loadedModule->load_addr = last_load_address;
    loadedModule->metadata = data;
    loadedModule->load_size = length;
    loadedModule->file_length = modfile->length;

    hashmap_set(module_hashmap, data->name, loadedModule);
    last_load_address += length;
    if (mdataout != NULL) memcpy(mdataout, data, sizeof(struct Metadata));
    return MODULE_OK;
}

static void module_handleFaultPriority(char *filename, char *priority) {
    if (!strcmp(priority, "REQUIRED")) {
        char *err = kmalloc(strlen("Could not load module ") + strlen(filename));
        strcpy(err, "Could not load module ");
        strcpy(err + strlen(err), filename);
        panic("module", "module_parseCFG", err);
    }

    if (!strcmp(priority, "HIGH")) {
        char *err = kmalloc(strlen("Could not load module (HIGH PRIOR) ") + strlen(filename));
        strcpy(err, "Could not load module (HIGH PRIOR) ");
        strcpy(err + strlen(err), filename);
        panic("module", "module_parseCFG", err);
    }

    serialPrintf("module_parseCFG: Module '%s' failed to load!\n", filename);
    printf("Failed to load module %s.\n", filename);
}

// module_parseCFG() - Will attempt to find and parse the module configuration, panics if it fails
void module_parseCFG() {
    // WARNING: Pretty hacky logic. This function should be called once because it will initialize the userspace too.

    // First, grab handles to root and/or initrd, depending on which has been mounted where.
    fsNode_t *root = open_file("/", 0);
    bool needsInitrd = (strcmp(root->name, "tarfs") == 0) ? false : true;
    fsNode_t *initrd = NULL;
    if (needsInitrd) initrd = open_file("/device/initrd/", 0); 

    // Next, grab the boottime and userspace configuration files (mod_boot.conf and mod_user.conf)
    // We ALWAYS want to pull from initial ramdisk and ONLY pull from backup root if necessary, because they can differ.
    fsNode_t *modBoot = (needsInitrd) ? open_file("/device/initrd/mod_boot.conf", 0) : open_file("/mod_boot.conf", 0);
    if (!modBoot) {
        if (needsInitrd) {
            // Try the backup method, but be warned that if the devices are different some modules may not be loaded.
            serialPrintf("module_parseCFG: WARNING!!!!! Pulling from backup device!!!\n");
            modBoot = open_file("/boot/conf/mod_boot.conf", 0);
        }

        if (!modBoot) {
            panic("module", "module_parseCFG", "mod_boot.conf not found");
        }
    }

    fsNode_t *modUser = (needsInitrd) ? open_file("/device/initrd/mod_user.conf", 0) : open_file("/mod_user.conf", 0);
    if (!modUser) {
        if (needsInitrd) {
            // Try the backup method, but be warned that if the devices are different some modules may not be loaded.
            serialPrintf("module_parseCFG: WARNING!!!!! Pulling from backup device!!!\n");
            modUser = open_file("/boot/conf/mod_user.conf", 0);
        }

        if (!modUser) {
            panic("module", "module_parseCFG", "mod_user.conf not found");
        }
    }


    // Okay, we've got the files we need. Read them in. 
    // BOOTTIME drivers MUST come from initial ramdisk because they are critical
    // We don't want to expose the boottime drivers to the problems of "make make_drive" being out-of-date
    // Userspace is more lax, and can be loaded from backups

    char *modboot_buf = kmalloc(modBoot->length);
    int ret = modBoot->read(modBoot, 0, modBoot->length, modboot_buf);
    if (ret != modBoot->length) {
        panic("module", "module_parseCFG", "Failed to read mod_boot.conf");
    }

    moduser_buf = kmalloc(modUser->length);
    ret = modUser->read(modUser, 0, modUser->length, moduser_buf);
    if (ret != modUser->length) {
        panic("module", "module_parseCFG", "Failed to read mod_user.conf");
    }

    // Let's start by initializing and parsing the boot-time drivers
    char *save;
    char *token = strtok_r(modboot_buf, "\n", &save);
    int line = 1;
    while (token != NULL) {
        if (!strcmp(token, "CONF_START")) goto _nextToken; // We are parsing a config file, who cares?
        if (!strcmp(token, "MOD_END"))  goto _nextToken;
        if (!strcmp(token, "CONF_END")) break;


        if (!strcmp(token, "MOD_START")) {
            // We are starting a module and now we will take control of the tokens.
            // Move to next token, this should be the filename.
            token = strtok_r(NULL, "\n", &save);
            line++;

            if (strstr(token, "FILENAME ") != token) {
                char *err = kmalloc(strlen("Parser error at line XXXX"));
                strcpy(err, "Parser error at line ");
                itoa(line, err + strlen(err), 10);
                panic("module", "module_parseCFG", err);
            }

            // To get the filename, find the first space and move past it by one
            char *filename = kmalloc(strlen(token));
            strcpy(filename, token + (strchr(token, ' ')-token) + 1);
            


            // Let's get the priority next
            token = strtok_r(NULL, "\n", &save);
            line++;

            if (strstr(token, "PRIORITY ") != token) {
                char *err = kmalloc(strlen("Parser error at line XXXX"));
                strcpy(err, "Parser error at line ");
                itoa(line, err + strlen(err), 10);
                panic("module", "module_parseCFG", err);
            }

            // To get the priority, find the first space and move past it by one
            char *priority = kmalloc(strlen(token));
            strcpy(priority, token + (strchr(token, ' ')-token) + 1);

            // First, let's make sure the module exists.
            char *fullpath = kmalloc(strlen("/boot/modules/") + strlen(filename));
            if (needsInitrd) strcpy(fullpath, "/boot/modules/");
            else strcpy(fullpath, "/modules/");
            strcpy(fullpath + strlen(fullpath), filename);

            serialPrintf("module_parseCFG: Loading module '%s' with priority %s...\n", fullpath, priority);
            fsNode_t *module = open_file(fullpath, 0);
            if (!module) {
                module_handleFaultPriority(filename, priority);
                goto _nextToken;
            }

            // Parse the module
            ret = module_load(module, 1, NULL, NULL);
            if (ret != MODULE_OK) {
                serialPrintf("module_parseCFG: module_load did not succeed, returned %i\n", ret);
                module_handleFaultPriority(filename, priority);
            } else {
                printf("Successfully loaded module '%s'.\n", filename);
            }

            // We r done
            goto _nextToken;
        }   

        _nextToken:
        token = strtok_r(NULL, "\n", &save);
        line++;
    }

    printf("Finish loading all modules.\n");
    kfree(modBoot);
    kfree(modboot_buf);
    kfree(modUser);
}

hashmap_t *module_getHashmap() {
    return module_hashmap;
}

// module_init() - Starts the module handler
void module_init() {
    module_hashmap = hashmap_create(10);
}