// ttydev.h - Header file for the TTY/PTY driver
// SOURCE OF THIS FILE IS TOARUOS. THE CODE ITSELF IS WRITTEN BY ME, BUT NOT THE STRUCTURE.

#ifndef TTYDEV_H
#define TTYDEV_H

// Includes
#include <kernel/ringbuffer.h>    // Ringbuffer support
#include <kernel/vfs.h>           // Virtual filesystem
#include <libk_reduced/ioctl.h>   // I/O Control
#include <libk_reduced/stdint.h>  // Integer declarations
#include <libk_reduced/termios.h> // Terminal interface

// PTY structure
typedef struct _pty {
    intptr_t name;       // PTY number
    fsNode_t* master;    // Master endpoint
    fsNode_t* slave;     // Slave endpoint
    struct winsize size; // Window size (width/height)
    struct termios tios; // termios data
    ringbuffer_t* in;    // In pipe
    ringbuffer_t* out;   // Out pipe
    char* canon_buffer;
    size_t canon_bufsize;
    size_t canon_buflen;
    int ct_proc; // Controlling process
    int fg_proc; // Foreground process

    void (*write_in)(struct _pty*, uint8_t);
    void (*write_out)(struct _pty*, uint8_t);

    int next_is_verbatim;

    void (*fill_name)(struct _pty*, char*);

    void* _private;
} pty_t;

// Functions
pty_t* tty_createPTY(struct winsize size);
void tty_init();

#endif
