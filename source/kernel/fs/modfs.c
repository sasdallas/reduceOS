// =======================================================================
// modfs.c - reduceOS driver to mount multiboot modules as filesystems
// =======================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/modfs.h>

// Only reading/writing has been implemented - there is no support for actual files.
// node->impl_struct will contain the multiboot module

static uint32_t modfs_read(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    uint32_t offset = (uint32_t)off;
    multiboot_mod_t *mod = node->impl_struct;
    uint32_t modSize = mod->mod_end - mod->mod_start;
    if (offset > modSize) return -1;

    // The usual fs sanity checks to make sure we won't overread
    uint32_t sizeToRead = 0;
    if (size + offset > modSize) sizeToRead = modSize-offset;
    else sizeToRead = size;

    // Just memcpy it in
    memcpy(buf, mod->mod_start + offset, sizeToRead);

    // Return size (even though we might not have read size)
    return size;
}

static uint32_t modfs_write(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    multiboot_mod_t *mod = node->impl_struct;
    uint32_t modSize = mod->mod_end - mod->mod_start;
    if (off > modSize) return -1;

    // The usual fs sanity checks to make sure we won't overread
    uint32_t sizeToWrite = 0;
    if (size + off > modSize) sizeToWrite = modSize - off;
    else sizeToWrite = size - off;

    // memcpy it
    memcpy(mod->mod_start + off, buf, sizeToWrite);
}

static uint32_t modfs_open(fsNode_t *node) {
    return 0;
}

static uint32_t modfs_close(fsNode_t *node) {
    return 0;
}


// Mounting functions (note that we don't do this the normal way)
void mountModfs(multiboot_mod_t *mod, char *mountpoint) {
    fsNode_t *out = kmalloc(sizeof(fsNode_t));
    out->impl_struct = mod;

    strcpy(out->name, mountpoint); // This may not be the best idea
    out->read = modfs_read;
    out->write = modfs_write;
    out->open = modfs_open;
    out->close = modfs_close;
    out->length = mod->mod_end - mod->mod_start;
    out->flags = VFS_BLOCKDEVICE;

    // TODO: Reserve memory?
    //vmm_allocateRegionFlags(mod->mod_start, mod->mod_start, out->length, 1, 0, 1);

    vfsMount(mountpoint, out);
}


void modfs_init(multiboot_info *info) {
    multiboot_mod_t *mod;
    int i;
    int modsMounted = 0;

    // Map the /device/modules/ directory
    vfs_mapDirectory("/device/modules");

    for (i = 0, mod = (multiboot_mod_t*)info->m_modsAddr; i < info->m_modsCount; i++, mod++) {
        // modfs command lines should start with modfs=1, and then their other types
        if (strstr(mod->cmdline, "modfs=1") == mod->cmdline) {
            char *mntpoint = kmalloc(strlen("/device/modules/modx"));
            strcpy(mntpoint, "/device/modules/mod");
            itoa(modsMounted, mntpoint+strlen("/device/modules/mod"), 10);

            mountModfs(mod, mntpoint);
            modsMounted++;
            kfree(mntpoint);
        }
    }
}