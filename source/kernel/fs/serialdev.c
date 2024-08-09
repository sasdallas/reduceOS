// ======================================================================================
// serialdev.c - Header file for the serial device
// ======================================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

// TODO: COM1-COM4 support

#include <kernel/serialdev.h> // Main header file


/* READ FUNCTIONS */

uint32_t serialdev_read(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    for (int i = 0; i < size; i++) {
        buf[i] = serialRead();
    }
    return size;
} 

/* WRITE FUNCTIONS */

uint32_t serialdev_write(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    uint8_t *actual_buffer = kmalloc(size);
    memcpy(actual_buffer, buf, size);
    serialPrintf("%s", actual_buffer);
    kfree(actual_buffer);
    return size;
}

/* OPEN/CLOSE FUNCTIONS */

void serialdev_open(fsNode_t *node) {
    return;
}

void serialdev_close(fsNode_t *node) {
    return;
}


/* INITIALIZATION FUNCTIONS */

static fsNode_t *get_serial_device() {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ret->open = serialdev_open;
    ret->close = serialdev_close;
    ret->read = serialdev_read;
    ret->write = serialdev_write;
    ret->flags = VFS_CHARDEVICE;

    ret->gid = ret->uid = ret->impl = ret->mask = 0;

    strcpy(ret->name, "Serial Output");

    return ret;
}

void serialdev_init() {
    vfsMount("/device/serial", get_serial_device());
}