// ============================================================
// console.c - Filesystem driver for a multipurpose console
// ============================================================
// This file is part of the reduceOS C kernel. Please credit me if you use this code.
// Basic implementation: https://github.com/klange/toaruos/blob/master/kernel/vfs/console.c

// TODO: This file should be integrated into debugdev, or vice versa. There is no need to have two of them.


#include <stdarg.h>
#include <kernel/process.h>
#include <kernel/vfs.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static fsNode_t *condev = NULL;
extern uint64_t boottime;
extern size_t (*printf_output)(size_t, uint8_t *);

static size_t (*console_write)(size_t, uint8_t *) = NULL;


static uint8_t tmp_buffer[4096] __attribute__((aligned(4096)));
static uint8_t *buffer_start = tmp_buffer;


// This isn't the VFS write method, it's just to write to the console itself or the buffer
static size_t write_console(size_t size, uint8_t *buffer) {
    if (console_write) return console_write(size, buffer); // The write method has been set

    if (buffer_start + size >= tmp_buffer + sizeof(tmp_buffer)) return 0;

    memcpy(buffer_start, buffer, size);
    buffer_start += size;
    return size;
}

struct printf_data {
    int previous_linefeed;
    int left_width;    
};

// Another cb_printf but minified
static int mini_cb_printf(void *user, char c) {
    struct printf_data *data = user;
    if (data->previous_linefeed) {
        for (int i = 0; i < data->left_width; i++) write_console(1, (uint8_t*)" ");
        data->previous_linefeed = 0;
    }

    if (c == '\n') data->previous_linefeed = 1;
    write_console(1, (uint8_t*)&c);
    return 0;
}

// console_setOutput(size_t (*output)(size_t, uint8_t*)) - Set the console output method
void console_setOutput(size_t (*output)(size_t,uint8_t*)) {
    console_write = output;
    if (buffer_start != tmp_buffer) {
        console_write(buffer_start - tmp_buffer, tmp_buffer);
        buffer_start = tmp_buffer;
    }
}

// console_printf(const char *fmt, ...) - Print out to the console
int console_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);


    struct printf_data _data = {0, 0};

    if (*fmt == '\a') {
        fmt++;
    } else {
        uint8_t seconds, minutes, hours, day, month;
        int year;

        // Get timestamp
        rtc_getDateTime(&seconds, &minutes, &hours, &day, &month, &year);

        char timestamp[100];
        int len = snprintf(timestamp, 99, "[%i/%i/%i %i:%i:%i] ", month, day, year, hours, minutes, seconds);   
        timestamp[len] = 0;
        _data.left_width = len;
        write_console(len, (uint8_t*)timestamp);
    }

    int out = xvasprintf(mini_cb_printf, &_data, fmt, args);
    va_end(args);
    return out;
}

// console_writeVFS(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) - Write for the VFS node
static size_t console_writeVFS(fsNode_t *node, off_t offset, uint32_t size, uint8_t *buffer) {
    if (size > 4096) return -1; // Nuh uh

    size_t size_in = size;
    if (size && *buffer == '\r') {
        write_console(1, (uint8_t*)"\r");
        buffer++;
        size--;
    }

    if (size) console_printf("%*s", (unsigned int)size, buffer);
    return size;
}

// console_create() - Create a filesystem node
static fsNode_t *console_create() {
    fsNode_t *ret = kmalloc(sizeof(fsNode_t));
    memset(ret, 0, sizeof(fsNode_t));
    ret->inode = ret->uid = ret->gid = 0;
    ret->mask = 0660;
    ret->flags = VFS_CHARDEVICE;
    ret->write = console_writeVFS;
    return ret;
}



// console_init() - Initialies the console
void console_init() {
    condev = console_create();
    vfsMount("/device/console", condev);
}