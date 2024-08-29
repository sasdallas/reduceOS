// =================================================================
// null.c - /device/null and /device/zero handler
// =================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/nulldev.h> // Main header file

/* READ METHODS */

uint32_t read_null(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buf) {
    memset(buf, NULL, size);
    return size;
}

uint32_t read_zero(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buf) {
    memset(buf, 0, size);
    return size;
}

/* WRITE METHODS */

uint32_t write_null(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buf) {
    return size;
}

uint32_t write_zero(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buf) {
    return size;
}

/* OPEN/CLOSE METHODS */

void open_null(fsNode_t *node) {
    return;
}

void close_null(fsNode_t *node) {
    return;
}

void open_zero(fsNode_t *node) {
    return;
}

void close_zero(fsNode_t *node) {
    return;
}

/* DEVICE CREATION */
static fsNode_t *get_null_device() {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ret->open = open_null;
    ret->close = close_null;
    ret->read = read_null;
    ret->write = write_null;
    ret->flags = VFS_CHARDEVICE;
    ret->uid = ret->gid = ret->impl = ret->mask = 0;

    strcpy(ret->name, "nulldev");

    return ret;
}

static fsNode_t *get_zero_device() {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ret->open = open_zero;
    ret->close = close_zero;
    ret->read = read_zero;
    ret->write = write_zero;
    ret->flags = VFS_CHARDEVICE;
    ret->uid = ret->gid = ret->impl = ret->mask = 0;

    strcpy(ret->name, "zerodev");

    return ret;
}


/* INSTALLATION METHODS */
void nulldev_init() {
    vfsMount("/device/null", get_null_device());
}

void zerodev_init() {
    vfsMount("/device/zero", get_zero_device());
}
