// ==============================================================================
// debugout.c - Provides /device/debug, which is the debugger output for serial
// ==============================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

#include <kernel/debugdev.h> // Main header file

static fsNode_t *debug_dev;
static fsNode_t *output_dev;

/* READ FUNCTIONS */
uint32_t debug_read(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    return 0;
}

/* WRITE FUNCTIONS */

uint32_t debug_write(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    uint8_t seconds, minutes, hours, day, month;
    int year;

    // Copy to external buffer (needed)
    char *buffer_to_write = kmalloc(size);
    uint32_t size_to_write = 0;
    if (size > strlen(buf)) size_to_write = strlen(buf);
    else size_to_write = size;
    memcpy(buffer_to_write, buf, size_to_write);


    // Get timestamp
    rtc_getDateTime(&seconds, &minutes, &hours, &day, &month, &year);

    char timestamp[100];
    int len = snprintf(timestamp, 99, "[%i/%i/%i %i:%i:%i] ", month, day, year, hours, minutes, seconds);   
    timestamp[len] = 0;

    // Open the serial device and write to it
    if (!output_dev) return;

    output_dev->write(output_dev, 0, len, timestamp);
    output_dev->write(output_dev, 0, size_to_write, buffer_to_write);
    output_dev->close(output_dev);

    kfree(buffer_to_write);
    return size;
}

/* OPEN/CLOSE FUNCTIONS */

void debug_open(fsNode_t *node) {
    return;
}

void debug_close(fsNode_t *node) {
    return;
}


/* INITIALIZATION FUNCTIONS */

static fsNode_t *get_debug_device() {
    if (debug_dev != NULL) return debug_dev;
    debug_dev = kmalloc(sizeof(fsNode_t));
    debug_dev->open = debug_open;
    debug_dev->close = debug_close;
    debug_dev->read = debug_read;
    debug_dev->write = debug_write;
    debug_dev->flags = VFS_CHARDEVICE;

    debug_dev->uid = debug_dev->gid = debug_dev->mask = debug_dev->impl = 0;
    
    strcpy(debug_dev->name, "Debug Output");

    return debug_dev;
}

void debugdev_init(fsNode_t *odev) {
    vfsMount("/device/debug", get_debug_device());
    output_dev = odev;
}