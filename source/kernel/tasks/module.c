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



// module_load(fsNode_t *modfile, int argc, char **args, struct Metadata *mdataout) - Loads a module for modfile
int module_load(fsNode_t *modfile, int argc, char **args, struct Metadata *mdataout) {
    if (!modfile || modfile->flags != VFS_FILE || modfile->length <= 1) return MODULE_PARAM_ERROR;

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


    serialPrintf("module_load: Loading module '%s'...\n", data->name);
    int status = data->init(argc, args);

    if (status != 0) {
        // Module failed to initialize, log the error and free it
        serialPrintf("module_load: Module '%s' failed to load correctly.\n", data->name);
        pmm_freeBlocks(mem, length);
        return MODULE_INIT_ERROR;
    }

    // We did it!
    last_load_address += length;
    memcpy(mdataout, data, sizeof(struct Metadata));
    return MODULE_OK;
}