// ======================================================================================
// serialdev.c - Header file for the serial device
// ======================================================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.

// TODO: COM1-COM4 support

#include <kernel/serialdev.h> // Main header file


/* READ FUNCTIONS */

uint32_t serialdev_read(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    // Get the COM port needed
    char *comPort = node->name;
    int switchret = -1;
    uint16_t oldCOM = serial_getCOM();

    if (!strcmp(comPort, "COM1")) switchret = serial_changeCOM(SERIAL_COM1);
    else if (!strcmp(comPort, "COM2")) switchret = serial_changeCOM(SERIAL_COM2);
    else if (!strcmp(comPort, "COM3")) switchret = serial_changeCOM(SERIAL_COM3);
    else if (!strcmp(comPort, "COM4")) switchret = serial_changeCOM(SERIAL_COM4);
    else serialPrintf("serialdev: Defaulting to this COM for unknown COM port %s\n", comPort);

    if (switchret != 0) {
        return 0;
    }
    

    for (uint32_t i = 0; i < size; i++) {
        buf[i] = serialRead();
    }

    serial_changeCOM(oldCOM);
    return size;
} 

/* WRITE FUNCTIONS */

uint32_t serialdev_write(fsNode_t *node, off_t off, uint32_t size, uint8_t *buf) {
    uint8_t *actual_buffer = kmalloc(size);
    memcpy(actual_buffer, buf, size);
    actual_buffer[size] = 0;

    // Get the COM port needed
    char *comPort = node->name;
    int switchret = -1;
    uint16_t oldCOM = serial_getCOM();

    if (!strcmp(comPort, "COM1")) switchret = serial_changeCOM(SERIAL_COM1);
    else if (!strcmp(comPort, "COM2")) switchret = serial_changeCOM(SERIAL_COM2);
    else if (!strcmp(comPort, "COM3")) switchret = serial_changeCOM(SERIAL_COM3);
    else if (!strcmp(comPort, "COM4")) switchret = serial_changeCOM(SERIAL_COM4);
    else serialPrintf("serialdev: Defaulting to this COM for unknown COM port %s\n", comPort);

    if (switchret != 0) {
        return 0;
    }

    
    serialPrintf("%s", actual_buffer);
    kfree(actual_buffer);

    serial_changeCOM(oldCOM);
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

static fsNode_t *get_serial_device(char *port) {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    ret->open = serialdev_open;
    ret->close = serialdev_close;
    ret->read = serialdev_read;
    ret->write = serialdev_write;
    ret->flags = VFS_CHARDEVICE;

    ret->gid = ret->uid = ret->impl = ret->mask = 0;

    strcpy(ret->name, port);

    return ret;
}

void serialdev_init() {
    uint16_t oldCOM = serial_getCOM();
    vfs_mapDirectory("/device/serial");
    
    // Run a test to see what serial ports are actually connected
    if (serial_changeCOM(SERIAL_COM1) == 0) {
        serialPrintf("==== PORT COM1 IDENTIFIED ====\n");
        serialPrintf("PORT MOUNTED AT /device/serial/COM1\n");
        serial_changeCOM(oldCOM);
        vfsMount("/device/serial/COM1", get_serial_device("COM1"));
    }

    if (serial_changeCOM(SERIAL_COM2) == 0) {
        serialPrintf("==== PORT COM2 IDENTIFIED ====\n");
        serialPrintf("PORT MOUNTED AT /device/serial/COM2\n");
        serial_changeCOM(oldCOM);
        vfsMount("/device/serial/COM2", get_serial_device("COM2"));
    }
    
    if (serial_changeCOM(SERIAL_COM3) == 0) {
        serialPrintf("==== PORT COM3 IDENTIFIED ====\n");
        serialPrintf("PORT MOUNTED AT /device/serial/COM3\n");
        serial_changeCOM(oldCOM);
        vfsMount("/device/serial/COM3", get_serial_device("COM3"));
    }
    
    if (serial_changeCOM(SERIAL_COM4) == 0) {
        serialPrintf("==== PORT COM4 IDENTIFIED ====\n");
        serialPrintf("PORT MOUNTED AT /device/serial/COM4\n");
        serial_changeCOM(oldCOM);
        vfsMount("/device/serial/COM4", get_serial_device("COM4"));
    }
}
